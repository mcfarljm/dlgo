#include <algorithm>
#include <iostream>

#include "agent_zero.h"
#include "../myrand.h"
#include "dihedral.h"


ZeroNode::ZeroNode(ConstGameStatePtr game_state, float value,
                   std::unordered_map<Move, float, MoveHash> priors,
                   std::weak_ptr<ZeroNode> parent,
                   std::optional<Move> last_move,
                   bool add_noise) :
  game_state(game_state), value(value), parent(parent), last_move(last_move),
  terminal(game_state->is_over()) {

  for (const auto &[move, p] : priors) {
    if (game_state->is_valid_move(move))
      branches.emplace(move, p);
  }

  assert((! branches.empty()) || terminal);

  if (add_noise && ! branches.empty()) {
    // std::cout << "Orig prior: ";
    // for (const auto &[move, p] : priors)
    //   std::cout << move << " " << p << ", ";
    // std::cout << std::endl;

    // Sample noise on legal moves:
    // Adjust concentration based on number of legal moves, following Katago
    // paper.
    double alpha = DIRICHLET_CONCENTRATION * 19.0 * 19.0 / branches.size();
    auto dirichlet_dist = DirichletDistribution(branches.size(), alpha);
    std::vector<double> noise = dirichlet_dist.sample();
    // std::cout << "Noise: " << noise << std::endl;

    size_t idx = 0;
    for (auto &[move, prior]: priors) {
      prior = (1.0 - DIRICHLET_WEIGHT) * prior +
        DIRICHLET_WEIGHT * noise[idx];
      ++idx;
    }

    // std::cout << "Noised prior: ";
    // for (const auto &[move, p] : priors)
    //   std::cout << move << " " << p << ", ";
    // std::cout << std::endl;
  }

  if (terminal) {
    // Override the model's value estimate with actual result
    ZeroNode::value = (game_state->next_player == game_state->winner().value()) ? 1.0 : -1.0;
  }
}


void ZeroNode::record_visit(Move move, float value) {
  ++total_visit_count;
  auto it = branches.find(move);
  assert(it != branches.end());
  (it->second.visit_count)++;
  (it->second.total_value) += value;
}


float ZeroNode::expected_value(Move m) const {
  auto branch = branches.find(m)->second;
  if (branch.visit_count == 0)
    return 0.0;
  return branch.total_value / branch.visit_count;
}


Move ZeroAgent::select_move(const GameState& game_state) {
  // std::cerr << "In select move, prior move count: " << game_state.num_moves << std::endl;
  auto root = create_node(std::make_shared<const GameState>(game_state));

  for (auto round_number=0; round_number < num_rounds; ++round_number) {
    // std::cout << "Round: " << round_number << std::endl;
    auto node = root;
    auto next_move = select_branch(*node);
    // std::cout << "Selected root move: " << next_move << std::endl;
    // for (auto it = node->children.find(next_move); it != node->children.end();) {
    for (std::unordered_map<Move, std::shared_ptr<ZeroNode>, MoveHash>::const_iterator it;
         it = node->children.find(next_move), it != node->children.end();) {
      node = it->second;
      if (node->terminal)
        break;
      next_move = select_branch(*node);
    }

    float value;
    std::optional<Move> move;
    if (! node->terminal) {
      auto new_state = node->game_state->apply_move(next_move);
      auto child_node = create_node(new_state, next_move, node);
      value = -1 * child_node->value;
      move = next_move;
    }
    else {
      value = node->value;
    }

    while (node) {
      if (node->terminal)
        (node->total_visit_count)++;
      else
        node->record_visit(move.value(), value);
      move = node->last_move; // Will be null at root node
      node = node->parent.lock();
      value = -1 * value;
    }
  }

  if (collector) {
    auto root_state_tensor = encoder->encode(game_state);
    auto visit_counts = torch::zeros(encoder->num_moves());
    auto get_visit_count = [&](int i) {
      auto move = encoder->decode_move_index(i);
      auto it = root->branches.find(move);
      if (it != root->branches.end())
        return it->second.visit_count;
      else
        return 0;
    };
    for (auto i=0; i < encoder->num_moves(); ++i) {
      visit_counts.index_put_({i}, static_cast<float>(get_visit_count(i)));
    }
    collector->record_decision(root_state_tensor, visit_counts);
  }

  int greedy_move_threshold = REFERENCE_GREEDY_MOVE_THRESHOLD * game_state.board->num_rows / 19;
  if (greedy || game_state.num_moves > greedy_move_threshold) {
      // Select the move with the highest visit count
      auto max_it = std::max_element(root->branches.begin(), root->branches.end(),
                                     [&root] (const auto& p1, const auto& p2) {
                                       return root->visit_count(p1.first) < root->visit_count(p2.first);
                                     });

      // for (const auto& [m, b] : root->branches)
      //   std::cerr << "visits: " << m << " " << b.visit_count << std::endl;
      // std::cerr << "E[V] = " << max_it->second.total_value / max_it->second.visit_count << ", visits = " << max_it->second.visit_count << std::endl;
      return max_it->first;
  }
  else {
    // Select move randomly in proportion to visit counts
    std::vector<Move> moves;
    std::vector<int> visit_counts;
    for (const auto& [move, branch] : root->branches) {
      moves.push_back(move);
      visit_counts.push_back(root->visit_count(move));
    }
    std::discrete_distribution<> dist(visit_counts.begin(), visit_counts.end());
    return moves[dist(rng)];
  }
}



std::shared_ptr<ZeroNode> ZeroAgent::create_node(ConstGameStatePtr game_state,
                                                 std::optional<Move> move,
                                                 std::weak_ptr<ZeroNode> parent) {

  // Note: also want to place this prior to loading jit model as well
  c10::InferenceMode guard;

  auto transform = Dihedral();
  auto state_tensor = encoder->encode(*game_state);
  // Random rotation or reflection:
  state_tensor = transform.forward(state_tensor);
  state_tensor.unsqueeze_(0);

  std::vector<torch::jit::IValue> input({state_tensor});
  auto output = model.forward(input);
  auto priors = output.toTuple()->elements()[0].toTensor(); // Shape: (1, num_moves)
  auto values = output.toTuple()->elements()[1].toTensor();

  // std::cout << "priors before transform: " << priors << std::endl;
  // std::cout << "priors before transform: " << priors.index({0, torch::indexing::Slice(0, 6)}) << std::endl;
  // Apply reverse transformation to the priors tensor.
  priors = encoder->untransform_policy(priors, transform);
  // std::cout << "priors after transform: " << priors << std::endl;

  values.squeeze_();
  priors.squeeze_();
  // std::cout << "priors size: " << at::numel(priors) << std::endl;
  auto value = values.item().toFloat();

  std::unordered_map<Move, float, MoveHash> move_priors;
  for (auto i=0; i<at::numel(priors); ++i) {
    move_priors.emplace(encoder->decode_move_index(i), priors.index({i}).item().toFloat());
  }

  auto new_node = std::make_shared<ZeroNode>(game_state, value,
                                             std::move(move_priors),
                                             parent,
                                             move,
                                             ! parent.lock());
  auto parent_shared = parent.lock();
  if (parent_shared) {
    assert(move);
    parent_shared->add_child(move.value(), new_node);
  }
  return new_node;
}



Move ZeroAgent::select_branch(const ZeroNode& node) const {
  auto score_branch = [&] (Move move) {
    auto q = node.expected_value(move);
    auto p = node.prior(move);
    auto n = node.visit_count(move);
    return q + c_uct * p * sqrt(node.total_visit_count) / (n + 1);
  };
  auto max_it = std::max_element(node.branches.begin(), node.branches.end(),
                                 [score_branch] (const auto& p1, const auto& p2) {
                                   return score_branch(p1.first) < score_branch(p2.first);
                                 });
  assert(max_it != node.branches.end());
  return max_it->first;
}
