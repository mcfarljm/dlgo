#include <iostream>
#include <chrono>
#include <thread>
#include <string>

#include "goboard.h"
#include "agent_naive.h"
#include "utils.h"

const auto CLEAR = "\033[2J\033[1;1H";


int main() {
  auto board_size = 6;
  auto game = GameState::new_game(board_size);
  auto bot = std::make_unique<RandomBot>();

  Move move = Move::pass();

  std::cout << CLEAR;
  std::cout << *game->board;

  while (! game->is_over()) {
    if (game->next_player == Player::black) {
      std::cout << "-- ";
      std::string input;
      std::cin >> input;
      auto point = point_from_coords(input);
      move = Move::play(point);
    }
    else
      move = bot->select_move(*game);

    std::cout << CLEAR;
    print_move(game->next_player, move);
    game = game->apply_move(move);
    std::cout << *game->board;
  }
    
}
