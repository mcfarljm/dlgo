import torch

class Dihedral:
    def __init__(self, i):
        assert 0 <= i <= 7
        self.flip = i > 3
        self.rotations = i % 4

    def forward(self, x):
        assert x.dim() == 3
        if self.rotations == 0 and (not self.flip):
            return x
        y = x
        if self.flip:
            y = y.flip(1)
        if self.rotations > 0:
            y = y.rot90(self.rotations, [1, 2])
        return y
            
    def inverse(self, x):
        assert x.dim() == 3
        if self.rotations == 0 and (not self.flip):
            return x
        y = x
        if self.rotations > 0:
            y = y.rot90(-self.rotations, [1, 2])
        if self.flip:
            y = y.flip(1)
        return y
