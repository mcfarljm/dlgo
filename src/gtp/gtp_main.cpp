#include <memory>
#include <iostream>

#include "frontend.h"
#include "../agent_naive.h"

int main(void) {
  
  std::cout << "Starting DLGO...\n";

  auto frontend = gtp::GTPFrontend(std::make_unique<RandomBot>());
  frontend.run();
}
