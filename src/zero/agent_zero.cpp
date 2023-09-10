#include <algorithm>
#include <iostream>

#include "agent_zero.h"


ZeroNode::ZeroNode(ConstGameStatePtr game_state, float value,
                   std::unordered_map<Move, float, MoveHash> priors,
                   std::weak_ptr<ZeroNode> parent,
                   std::optional<Move> last_move) :
  game_state(game_state), value(value), parent(parent), last_move(last_move),
  terminal(game_state->is_over()) {

  for (const auto &[move, p] : priors) {
    if (game_state->is_valid_move(move))
      branches.emplace(move, p);
  }

  assert((! branches.empty()) || terminal);

  if (terminal)
    // Override the model's value estimate with actual result
    value = (game_state->next_player == game_state->winner().value()) ? 1.0 : -1.0;
}


void ZeroNode::record_visit(Move move, float value) {
  ++total_visit_count;
  auto it = branches.find(move);
  assert(it != branches.end());
  (it->second.visit_count)++;
  (it->second.total_value) += value;
}


float ZeroNode::expected_value(Move m) {
  auto branch = branches.find(m)->second;
  if (branch.visit_count == 0)
    return 0.0;
  return branch.total_value / branch.visit_count;
}


Move ZeroAgent::select_move(const GameState& game_state) {
  // std::cout << "In select move\n";
  auto root = create_node(std::make_shared<const GameState>(game_state));

  for (auto round_number=0; round_number < num_rounds; ++round_number) {
    // std::cout << "Round: " << round_number << std::endl;
    auto node = root;
    auto next_move = select_branch(node);
    // std::cout << "Selected root move: " << next_move << std::endl;
    // for (auto it = node->children.find(next_move); it != node->children.end();) {
    for (std::unordered_map<Move, std::shared_ptr<ZeroNode>, MoveHash>::const_iterator it;
         it = node->children.find(next_move), it != node->children.end();) {
      node = it->second;
      if (node->terminal)
        break;
      next_move = select_branch(node);
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

  // Todo: Option to select proportionally with temperature, as in AGZ paper.
  auto max_it = std::max_element(root->branches.begin(), root->branches.end(),
                                 [&root] (const auto& p1, const auto& p2) {
                                   return root->visit_count(p1.first) < root->visit_count(p2.first);
                                 });

  return max_it->first;
}



std::shared_ptr<ZeroNode> ZeroAgent::create_node(ConstGameStatePtr game_state,
                                                 std::optional<Move> move,
                                                 std::weak_ptr<ZeroNode> parent) {

  // Note: also want to place this prior to loading jit model as well
  c10::InferenceMode guard;
  
  auto state_tensor = encoder->encode(*game_state);
  std::vector<torch::jit::IValue> input({state_tensor});
  // Todo: verify that unsqueeze_ really works in place
  state_tensor.unsqueeze_(0);
  auto output = model.forward(input);
  auto priors = output.toTuple()->elements()[0].toTensor();
  auto values = output.toTuple()->elements()[1].toTensor();
  values.squeeze_();
  priors.squeeze_();
  // std::cout << "priors size: " << at::numel(priors) << std::endl;
  auto value = values.item().toFloat();

  // Todo: add dirichlet noise

  std::unordered_map<Move, float, MoveHash> move_priors;
  for (auto i=0; i<at::numel(priors); ++i) {
    move_priors.emplace(encoder->decode_move_index(i), priors.index({i}).item().toFloat());
  }

  auto new_node = std::make_shared<ZeroNode>(game_state, value, std::move(move_priors), parent, move);
  auto parent_shared = parent.lock();
  if (parent_shared) {
    assert(move);
    parent_shared->add_child(move.value(), new_node);
  }
  return new_node;
}



Move ZeroAgent::select_branch(std::shared_ptr<ZeroNode> node) {
  auto score_branch = [&] (Move move) {
    auto q = node->expected_value(move);
    auto p = node->prior(move);
    auto n = node->visit_count(move);
    return q + c_uct * p * sqrt(node->total_visit_count) / (n + 1);
  };
  auto max_it = std::max_element(node->branches.begin(), node->branches.end(),
                                 [score_branch] (const auto& p1, const auto& p2) {
                                   return score_branch(p1.first) < score_branch(p2.first);
                                 });
  assert(max_it != node->branches.end());
  return max_it->first;
}
