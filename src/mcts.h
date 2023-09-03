#ifndef MCTS_H
#define MCTS_H

#include <vector>
#include <memory>
#include <algorithm>

#include "goboard.h"
#include "myrand.h"
#include "agent_base.h"

class MCTSNode;
using MCTSNodePtr = std::shared_ptr<MCTSNode>;
using ConstMCTSNodePtr = std::shared_ptr<const MCTSNode>;


class MCTSNode : public std::enable_shared_from_this<MCTSNode> {
  int win_counts[2] = {0, 0};
  std::vector<Move> legal_moves;
  /* To avoid the cost of removing from random positions in the vector
  legal_moves, we keep an index of unvisited moves.  It is randomized initially
  so that we can pop off the back efficiently. */
  std::vector<size_t> unvisited_moves;

public:
  ConstGameStatePtr game_state;
  std::vector<MCTSNodePtr> children;
  std::weak_ptr<MCTSNode> parent;
  int num_rollouts = 0;
  std::optional<Move> move;

  MCTSNode(ConstGameStatePtr game_state,
           std::weak_ptr<MCTSNode> parent = std::weak_ptr<MCTSNode>(),
           std::optional<Move> move = std::nullopt) :
    game_state(game_state), parent(parent), move(move) {
    legal_moves = game_state->legal_moves();
    for (size_t i=0; i<legal_moves.size(); ++i)
      unvisited_moves.push_back(i);
    std::shuffle(unvisited_moves.begin(), unvisited_moves.end(), rng);
  }

  std::shared_ptr<MCTSNode> add_random_child();

  void record_win(Player winner) {
    ++win_counts[int(winner)];
    ++num_rollouts;
  }

  bool can_add_child() const {
    return ! unvisited_moves.empty();
  }

  bool is_terminal() const {
    return game_state->is_over();
  }

  float winning_frac(Player player) const {
    return float(win_counts[int(player)]) / float(num_rollouts);
  }

};

class MCTSAgent : public Agent {
  int num_rounds;
  float temperature;
public:
  MCTSAgent(int num_rounds, float temperature) :
    num_rounds(num_rounds), temperature(temperature) {}

  Move select_move(const GameState&);

  static Player simulate_random_game(ConstGameStatePtr);

private:
  MCTSNodePtr select_child(MCTSNodePtr node);
  
};

#endif // MCTS_H
