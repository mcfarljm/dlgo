#include "eval.h"

int capture_diff(const GameState& game_state) {
  int black_stones = 0;
  int white_stones = 0;
  for (auto r=1; r <= game_state.board->num_rows; ++r) {
    for (auto c=1; c <= game_state.board->num_cols; ++c) {
      auto color = game_state.board->get(Point(r, c));
      if (color == Player::black)
        ++black_stones;
      else if (color == Player::white)
        ++white_stones;
    }
  }
  auto diff = black_stones - white_stones;
  if (game_state.next_player == Player::black)
    return diff;
  return -1 * diff;
}
