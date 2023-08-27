#include <iostream>
#include <chrono>
#include <thread>
#include <string>

#include "goboard.h"
#include "agent_naive.h"

const auto CLEAR = "\033[2J\033[1;1H";


// void print_move(Player player, Move move) {
//   std::string move_str;
//   if (move.is_pass)
//     move_str = "passes";
//   else if (move.is_resign)
//     move_str = "resigns";
//   else
//     move_str = 


int main() {
  auto board_size = 9;
  auto game = GameState::new_game(board_size);

  auto x = std::make_unique<RandomBot>();

  std::vector<std::unique_ptr<Agent>> bots;
  bots.push_back(std::make_unique<RandomBot>());
  bots.push_back(std::make_unique<RandomBot>());

  while (! game->is_over()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << CLEAR;
    game->board->print();
    auto bot_move = bots[size_t(game->next_player)]->select_move(*game);
    game = game->apply_move(bot_move);
  }
    
}
