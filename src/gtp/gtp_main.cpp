#include <memory>
#include <iostream>

#include <cxxopts.hpp>

#include "frontend.h"
#include "../agent_naive.h"
#include "../zero/agent_zero.h"
#include "../alphabeta.h"
#include "../eval.h"
#include "../mcts.h"


// Todo: move these to a shared file, they are also used by matchup
std::unique_ptr<Agent> load_zero_agent(const std::string network_path,
                                       int board_size,
                                       int num_rounds) {
  c10::InferenceMode guard;
  torch::jit::script::Module model;
  try {
    model = torch::jit::load(network_path);
  }
  catch (const c10::Error& e) {
    std::cerr << "Error loading model: " << network_path << std::endl;
    // Return emptpy pointer
    return std::unique_ptr<Agent>();
  }

  std::cout << "Loaded: " << network_path << std::endl;

  auto encoder = std::make_shared<SimpleEncoder>(board_size);
  auto agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, true);
  return agent;
}


std::unique_ptr<Agent> load_agent(const std::string identifier,
                                       int board_size,
                                       int num_rounds) {
  if (identifier == "random") {
    std::cout << "loading random agent" << std::endl;
    return std::make_unique<FastRandomBot>();
  }
  else if (identifier == "mcts") {
    std::cout << "loading mcts agent with " << num_rounds << " rounds" << std::endl;
    return std::make_unique<MCTSAgent>(num_rounds, 1.5);
  }
  // auto frontend = gtp::GTPFrontend(std::make_unique<AlphaBetaAgent>(2, &capture_diff));
  else
    return load_zero_agent(identifier, board_size, num_rounds);
}

int main(int argc, const char* argv[]) {

  cxxopts::Options options("dlgobot", "Run engine using GTP");

  options.add_options()
    ("agent", "Agent identifier or network file", cxxopts::value<std::string>()->default_value("mcts"))
    ("r,rounds", "Number of rounds", cxxopts::value<int>()->default_value("800"))
    ("t,num-threads", "Number of pytorch threads", cxxopts::value<int>())
    ("h,help", "Print usage")
    ;

  cxxopts::ParseResult args;
  try {
    args = options.parse(argc, argv);
  }
  catch (const cxxopts::exceptions::exception& e) {
    std::cout << options.help() << std::endl;
    exit(1);
  }

  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  auto num_rounds = args["rounds"].as<int>();
  
  std::cout << "Starting DLGO...\n";

  auto agent = load_agent(args["agent"].as<std::string>(),
                           9, num_rounds);

  auto frontend = gtp::GTPFrontend(std::move(agent));

  frontend.run();
}
