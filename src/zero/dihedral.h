#ifndef DIHEDRAL_H
#define DIHEDRAL_H

#include <torch/torch.h>

#include "../myrand.h"

/// Define dihedral rotations and reflections of the board tensors.

/// The 8 possible transformations are:
/// - 0, 1, 2, or 3 90-degree rotations
/// - flip and 0, 1, 2, or 3 90-degree rotations
/// (the flip could be either up-down or left-right)
class Dihedral {

  int rotations;
  bool flip;

public:

  /// Pick a random transformation
  Dihedral() {
    std::uniform_int_distribution<> dist(0, 7);
    auto i = dist(rng);
    flip = (i > 3);
    rotations = i % 4;
  }

  /// Specified transformation, for testing
  Dihedral(int rotations, bool flip) : rotations{rotations}, flip{flip} {}

  torch::Tensor forward(const torch::Tensor& input_tensor) const {
    assert(input_tensor.dim() == 3);
    if (rotations == 0 && (! flip))
      return input_tensor;
    auto new_tensor = input_tensor;
    if (flip)
      new_tensor = torch::flip(new_tensor, {1});
    if (rotations)
      new_tensor = torch::rot90(new_tensor, rotations, {1, 2});
    return new_tensor;
  }

  torch::Tensor inverse(const torch::Tensor& input_tensor) const {
    assert(input_tensor.dim() == 3);
    if (rotations == 0 && (! flip))
      return input_tensor;
    auto new_tensor = input_tensor;
    if (rotations)
      new_tensor = torch::rot90(new_tensor, -rotations, {1, 2});
    if (flip)
      new_tensor = torch::flip(new_tensor, {1});
    return new_tensor;
  }

};


#endif // DIHEDRAL_H
