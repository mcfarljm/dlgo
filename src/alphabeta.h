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


// Public for testing
int alpha_beta_result(const GameState& game_state,
                      int max_depth, int best_black, int best_white,
                      int (*eval_fn)(const GameState&));

#endif // ALPHA_BETA_H
