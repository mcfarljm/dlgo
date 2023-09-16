#include <algorithm>

#include "encoder.h"

using namespace torch::indexing;

/// Simple 11-plane encoder from book
/// Stones encoded by liberty count from perspective of current player
///
/// 0 - 3: Current player's stones with 1, 2, 3, 4+ liberties
/// 4 - 7: Opponent's stones with 1, 2, 3, 4+ liberties
/// 8: 1 if white to move (current player gets komi)
/// 9: 1 if black to move (opponent gets komi)
/// 10: move would be illegal due to ko
torch::Tensor SimpleEncoder::encode(const GameState& game_state) const {
  auto board_tensor = torch::zeros({11, board_size, board_size});
  auto next_player = game_state.next_player;

  if (next_player == Player::white)
    board_tensor.index_put_({8, Ellipsis}, 1.0);
  else
    board_tensor.index_put_({9, Ellipsis}, 1.0);
  for (auto i=0; i<board_size; ++i) {
    for (auto j=0; j<board_size; ++j) {
      auto p = Point(i+1, j+1);
      auto go_string = game_state.board->get_go_string(p);

      if (! go_string) {
        if (game_state.does_move_violate_ko(next_player, Move::play(p)))
          board_tensor.index_put_({10, i, j}, 1.0);
      }
      else {
        auto liberty_plane = std::min(4, go_string.value()->num_liberties()) - 1;
        if (go_string.value()->color != next_player)
          liberty_plane += 4;
        board_tensor.index_put_({liberty_plane, i, j}, 1.0);
      }
    }
  }
  
  return board_tensor;
}

Move SimpleEncoder::decode_move_index(int index) const {
  if (index == board_size * board_size)
    return Move::pass();
  auto row = index / board_size;
  auto col = index % board_size;
  return Move::play(Point(row+1, col+1));
}


torch::Tensor SimpleEncoder::untransform_policy(const torch::Tensor policy, const Dihedral dihedral) const {
  assert(policy.dim() == 2);
  auto board_moves = policy.index({Slice(), Slice(0, -1)});
  assert(torch::numel(board_moves) == board_size * board_size);
  board_moves = board_moves.reshape({1, board_size, board_size});
  board_moves = dihedral.inverse(board_moves).reshape({1, board_size * board_size});
  // Now add back the pass element to the end
  return torch::cat({board_moves, policy.index({Slice(), Slice(-1, None)})}, 1);
}
