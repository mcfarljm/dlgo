#include "simulation.h"

#include <iostream>
#include "goboard.h"
#include "utils.h"
#include "scoring.h"


std::pair<Player, int> simulate_game(int board_size,
                                     Agent* black_agent,
                                     Agent* white_agent,
                                     int verbosity,
                                     int max_moves) {
  Agent* agents[2] = {black_agent, white_agent};
  int move_count = 0;

  auto game = GameState::new_game(board_size);

  while (move_count < max_moves && ! game->is_over()) {
    if (verbosity >= 3)
      std::cout << *game->board;
    auto move = agents[size_t(game->next_player)]->select_move(*game);
    if (verbosity >= 2)
      print_move(game->next_player, move);
    game = game->apply_move(move);
    ++move_count;
  }

  auto game_result = GameResult(game->board);
  auto winner = game_result.winner();
  if (verbosity >= 1) {
    std::cout << move_count << " moves\n";
    std::cout << "Winner: " << winner << std::endl;
  }
  return std::make_pair(winner, move_count);
}
