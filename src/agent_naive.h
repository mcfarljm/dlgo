#ifndef AGENT_NAIVE_H
#define AGENT_NAIVE_H

#include <random>
#include "agent_base.h"
#include "agent_helpers.h"
#include "myrand.h"


class RandomBot : public Agent {
public:
  /// Choose a random valid move that preserves our own eyes.
  Move select_move(const GameState& game_state) {
    std::vector<Point> candidates;
    for (auto r=1; r <= game_state.board->num_rows; r++) {
      for (auto c=1; c <= game_state.board->num_cols; c++) {
        auto candidate = Point(r, c);
        if (game_state.is_valid_move(Move::play(candidate)) &&
            ! is_point_an_eye(*game_state.board, candidate, game_state.next_player))
          candidates.push_back(candidate);
      }
    }
    if (candidates.empty())
      return Move::pass();
    
    std::uniform_int_distribution<> dist(0, candidates.size() - 1);
    auto idx = dist(rng);
    return Move::play(candidates[idx]);
  }
};

#endif // AGENT_NAIVE_H
