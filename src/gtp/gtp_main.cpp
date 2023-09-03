#include <memory>
#include <iostream>

#include "frontend.h"
#include "../agent_naive.h"
#include "../alphabeta.h"
#include "../eval.h"
#include "../mcts.h"

int main(void) {
  
  std::cout << "Starting DLGO...\n";

  // auto frontend = gtp::GTPFrontend(std::make_unique<RandomBot>());
  // auto frontend = gtp::GTPFrontend(std::make_unique<AlphaBetaAgent>(2, &capture_diff));
  auto frontend = gtp::GTPFrontend(std::make_unique<MCTSAgent>(2000, 1.5));
  frontend.run();
}
