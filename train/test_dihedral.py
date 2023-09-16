import unittest

import torch

from dihedral import Dihedral


class TestDihedral(unittest.TestCase):
    def test_dihedral(self):
        x = torch.arange(8).reshape(2,2,2)

        for i in range(8):
            transform = Dihedral(i)
            # print(transform.forward(x))
            self.assertTrue(torch.equal(x, transform.inverse(transform.forward(x))))
