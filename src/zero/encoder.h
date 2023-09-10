#ifndef ENCODER_H
#define ENCODER_H

#include <torch/torch.h>

#include "../goboard.h"


class Encoder {
 public:
  virtual torch::Tensor encode(const GameState&) const = 0;
  virtual Move decode_move_index(int index) const = 0;
  virtual int num_moves() const = 0;
};

class SimpleEncoder : public Encoder {
  int board_size;
  constexpr static int num_planes = 11;

 public:
  SimpleEncoder(int board_size) : board_size(board_size) {}
  
  torch::Tensor encode(const GameState&) const;
  Move decode_move_index(int index) const;
  int num_moves() const {
    return board_size * board_size + 1;
  }
};


#endif // ENCODER_H
