#ifndef ALPHA_BETA_H
#define ALPHA_BETA_H

#include "agent_base.h"

class AlphaBetaAgent : public Agent {
  int max_depth;
  int (*eval_fn)(const GameState&);
public:

  AlphaBetaAgent(int max_depth, int (*eval_fn)(const GameState&)) :
    max_depth(max_depth), eval_fn(eval_fn) {}

  Move select_move(const GameState& game_state);

};

#endif // ALPHA_BETA_H
