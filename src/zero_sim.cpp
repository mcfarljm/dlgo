#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <array>

#include "goboard.h"
#include "zero/agent_zero.h"
#include "utils.h"
#include "scoring.h"


std::pair<Player, int> simulate_game(int board_size,
                     Agent* black_agent,
                     Agent* white_agent) {
  Agent* agents[2] = {black_agent, white_agent};
  int move_count = 0;

  auto game = GameState::new_game(board_size);

  while (! game->is_over()) {
    std::cout << *game->board;
    auto move = agents[size_t(game->next_player)]->select_move(*game);
    print_move(game->next_player, move);
    game = game->apply_move(move);
    ++move_count;
  }

  auto game_result = GameResult(game->board);
  auto winner = game_result.winner();
  std::cout << move_count << " moves\n";
  std::cout << "Winner: " << int(winner) << std::endl;
  return std::make_pair(winner, move_count);
}


int main(int argc, const char* argv[]) {
  constexpr auto board_size = 9;
  constexpr auto num_rounds = 800;

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

  auto black_agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, false);
  auto white_agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, false);

  auto timer = Timer();
  auto [result, num_moves] = simulate_game(board_size, black_agent.get(), white_agent.get());
  auto duration = timer.elapsed();
  std::cout << "Time: " << timer.elapsed() << std::endl;
  std::cout << "Moves per second: " << num_moves / duration << std::endl;
  std::cout << "Seconds per move: " << duration / num_moves << std::endl;
}
