#ifndef ENCODER_H
#define ENCODER_H

#include <torch/torch.h>

#include "../goboard.h"


class Encoder {
 public:
  virtual torch::Tensor encode(const GameState&) = 0;
  virtual Move decode_move_index(int index) = 0;
};

class SimpleEncoder : public Encoder {
  int board_size;
  constexpr static int num_planes = 11;

 public:
  SimpleEncoder(int board_size) : board_size(board_size) {}
  
  torch::Tensor encode(const GameState&);
  Move decode_move_index(int index);
};


#endif // ENCODER_H
