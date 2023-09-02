#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <array>

#include "goboard.h"
#include "agent_naive.h"
#include "utils.h"

const auto CLEAR = "\033[2J\033[1;1H";


int main() {
  auto board_size = 9;
  auto game = GameState::new_game(board_size);

  std::array<std::unique_ptr<Agent>, 2> bots = {
    std::make_unique<RandomBot>(),
    std::make_unique<RandomBot>(),
  };

  while (! game->is_over()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << CLEAR;
    std::cout << *game->board;
    auto bot_move = bots[size_t(game->next_player)]->select_move(*game);
    print_move(game->next_player, bot_move);
    game = game->apply_move(bot_move);
  }
    
}
