#ifndef AGENT_NAIVE_H
#define AGENT_NAIVE_H

#include <random>
#include "agent_base.h"
#include "agent_helpers.h"
#include "myrand.h"


class RandomBot : public Agent {
public:
  /// Choose a random valid move that preserves our own eyes.
  Move select_move(const GameState& game_state);
};

class FastRandomBot : public Agent {
public:
  Move select_move(const GameState& game_state);
private:
  std::pair<int, int> cached_dim = {0, 0};
  std::vector<Point> point_cache;
  void update_cache(std::pair<int, int> dim);
};

#endif // AGENT_NAIVE_H
