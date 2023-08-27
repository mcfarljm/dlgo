#ifndef AGENT_BASE_H
#define AGENT_BASE_H

#include "goboard.h"

class Agent {
 public:
  virtual Move select_move(const GameState&) = 0;
};

#endif // AGENT_BASE_H
