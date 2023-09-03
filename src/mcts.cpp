#include <algorithm>
#include <cmath>
#include <iostream>

#include "mcts.h"
#include "agent_naive.h"


std::shared_ptr<MCTSNode> MCTSNode::add_random_child() {
  // The array of indices has been randomly shuffled, so we just pop from the
  // back.
  auto move_index = unvisited_moves.back();
  unvisited_moves.pop_back();
  auto new_move = legal_moves[move_index];
  auto new_game_state = game_state->apply_move(new_move);
  auto new_node = std::make_shared<MCTSNode>(new_game_state, weak_from_this(), new_move);
  children.push_back(new_node);
  return new_node;
}



Move MCTSAgent::select_move(const GameState& game_state)  {
  auto root = std::make_shared<MCTSNode>(std::make_shared<const GameState>(game_state));

  for (auto i=0; i<num_rounds; ++i) {
    // std:: cout << "Round: " << i << std::endl;
    auto node = root;
    while ((! node->can_add_child()) && (! node->is_terminal()))
      node = select_child(node);

    // Add a new child node into the tree.
    if (node->can_add_child())
      node = node->add_random_child();

    // Simulate a random game from this node.
    auto winner = simulate_random_game(node->game_state);

    // Propagate scores back up the tree.
    while (true) {
      node->record_win(winner);
      if (node->parent.expired())
        break;
      node = MCTSNodePtr(node->parent); // Weak ptr to shared ptr
    }
  }

  // Having performed the MCTS rounds, we now pick a move.
  auto best_move = Move::pass();
  float best_pct = -1.0;
  for (const auto& child : root->children) {
    auto child_pct = child->winning_frac(game_state.next_player);
    if (child_pct > best_pct) {
      best_pct = child_pct;
      best_move = child->move.value();
    }
  }
  return best_move;
}



/// Select a child according to the upper confidence bound for trees (UCT)
/// metric.
MCTSNodePtr MCTSAgent::select_child(MCTSNodePtr node) {
  assert(! node->children.empty());
  auto total_rollouts = std::accumulate(node->children.begin(),
                                        node->children.end(),
                                        0,
                                        [](int total, auto b){return total + b->num_rollouts;});
  auto log_rollouts = log(total_rollouts);

  auto best_score = -1;
  MCTSNodePtr best_child;
  for (const auto& child : node->children) {
    // Calculate the UCT score.
    auto win_percentage = child->winning_frac(node->game_state->next_player);
    auto exploration_factor = sqrt(log_rollouts / child->num_rollouts);
    auto uct_score = win_percentage + temperature * exploration_factor;
    // Check if this is the largest we've seen so far.
    if (uct_score > best_score) {
      best_score = uct_score;
      best_child = child;
    }
  }
  return best_child;
}

Player MCTSAgent::simulate_random_game(ConstGameStatePtr game) {
  std::array<std::unique_ptr<Agent>, 2> bots = {
    std::make_unique<FastRandomBot>(),
    std::make_unique<FastRandomBot>(),
  };

  while (! game->is_over()) {
    auto move = bots[int(game->next_player)]->select_move(*game);
    game = game->apply_move(move);
  }
  return game->winner().value();
}
