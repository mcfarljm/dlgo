#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <array>

#include <cxxopts.hpp>

#include "goboard.h"
#include "zero/agent_zero.h"
#include "utils.h"
#include "scoring.h"
#include "simulation.h"


int main(int argc, const char* argv[]) {

  constexpr auto board_size = 9;

  cxxopts::Options options("zero_sim", "Simulate games using zero agent");

  options.add_options()
    ("network", "Path to pytorch script file", cxxopts::value<std::string>())
    ("r,rounds", "Number of rounds", cxxopts::value<int>()->default_value("800"))
    ("v,verbosity", "Verbosity level", cxxopts::value<int>()->default_value("1"))
    ("t,num-threads", "Number of pytorch threads", cxxopts::value<int>())
    ("h,help", "Print usage")
    ;

  options.parse_positional({"network"});
  options.positional_help("<network_file>");

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

  if (! args.count("network")) {
    std::cout << options.help() << std::endl;
    exit(1);
  }

  auto num_rounds = args["rounds"].as<int>();
  auto verbosity = args["verbosity"].as<int>();
    
  if (args.count("num-threads")) {
    std::cout << "setting " << args["num-threads"].as<int>() << " pytorch threads" << std::endl;
    at::set_num_threads(args["num-threads"].as<int>());
  }

  c10::InferenceMode guard;
  torch::jit::script::Module model;
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    model = torch::jit::load(args["network"].as<std::string>());
  }
  catch (const c10::Error& e) {
    std::cerr << "error loading the model\n";
    return -1;
  }

  std::cout << "Model loaded\n";

  auto encoder = std::make_shared<SimpleEncoder>(board_size);

  auto black_collector = std::make_shared<ExperienceCollector>();
  auto white_collector = std::make_shared<ExperienceCollector>();

  auto black_agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, false);
  auto white_agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, false);

  black_agent->set_collector(black_collector);
  white_agent->set_collector(white_collector);

  auto timer = Timer();
  auto [winner, num_moves] = simulate_game(board_size, black_agent.get(), white_agent.get(), verbosity);
  auto duration = timer.elapsed();
  std::cout << "Time: " << timer.elapsed() << std::endl;
  std::cout << "Moves per second: " << num_moves / duration << std::endl;
  std::cout << "Seconds per move: " << duration / num_moves << std::endl;

  auto black_reward = winner == Player::black ? 1.0 : -1.0;
  black_collector->complete_episode(black_reward);
  white_collector->complete_episode(-1.0 * black_reward);

  std::cout << "Experience: " << black_collector->states.size() << " " <<
    black_collector->visit_counts.size() << std::endl;
  std::cout << black_collector->rewards << std::endl;

  black_collector->append(*white_collector);
  black_collector->serialize_binary("experience");
}
