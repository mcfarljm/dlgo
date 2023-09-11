#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <array>

#include "goboard.h"
#include "zero/agent_zero.h"
#include "utils.h"
#include "scoring.h"
#include "simulation.h"



int main(int argc, const char* argv[]) {
  constexpr auto board_size = 9;
  constexpr auto num_rounds = 800;
  constexpr auto verbosity = 3;

  if (argc != 2) {
    std::cerr << "usage: prog <path-to-exported-script-module>\n";
    return -1;
  }

  c10::InferenceMode guard;
  torch::jit::script::Module model;
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    model = torch::jit::load(argv[1]);
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
  black_collector->serialize("experience.pt");
}
