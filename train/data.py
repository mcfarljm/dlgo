import os
import glob
import json

import numpy as np
import torch
from torch.utils.data import Dataset, ConcatDataset, Subset

from dihedral import Dihedral


DATATYPES = {'float32': np.float32,
             'int8': np.int8,
             'int16': np.int16}


class ExperienceSubset(Subset):
    """Random subset of items from all experience files in a directory."""
    def __init__(self, directory, n):
        dataset = ExperienceSet(directory)
        print(f'loaded augmented experience data with {len(dataset)} moves')
        if n < 1:
            # Treat as fraction
            n = int(len(dataset) * n)
        if n > len(dataset):
            raise ValueError
        print(f'using subset of {n} moves')
        indices = np.random.choice(len(dataset), n, replace=False)
        super().__init__(dataset, indices)


class ExperienceSet(ConcatDataset):
    """Concatenate all experience files in a given directory."""
    def __init__(self, directory):
        labels = [s.split('states')[1].split('.json')[0] for s in
                  glob.glob(os.path.join(directory, 'states*.json'))]
        triples = [[os.path.join(directory, f'{key}{label}.json')
                    for key in ('states', 'rewards', 'visit_counts')]
                   for label in labels]
        datasets = [AugmentedExperienceChunk(*triple) for triple in triples]
        super().__init__(datasets)


class ExperienceChunk(Dataset):
    """Experience data for an individual item.

    Intent is for these to be collected using ChainDataSet.
    """
    def __init__(self, states_path, rewards_path, visit_counts_path):
        """Args:
        path (str): Path to states json file.
        """
        self.states_path = states_path
        self.rewards_path = rewards_path
        self.visit_counts_path = visit_counts_path
        self.dirname = os.path.dirname(states_path)
        assert self.dirname == os.path.dirname(rewards_path) == os.path.dirname(visit_counts_path)

        with open(states_path, 'r') as f:
            self.states_info = json.load(f)
        with open(rewards_path, 'r') as f:
            self.rewards_info = json.load(f)
        with open(visit_counts_path, 'r') as f:
            self.visit_counts_info = json.load(f)

        self.num_moves = self.states_info['shape'][0]
        strides = self.states_info['strides']
        assert strides[0] > strides[1] > strides[2] > strides[3]

    def _memmap_data(self, info):
        return np.memmap(os.path.join(self.dirname, info['data']),
                         dtype=DATATYPES[info['dtype']],
                         mode='r',
                         shape=tuple(info['shape']))

    def __len__(self):
        return self.num_moves

    def __getitem__(self, idx):

        memmaps = [self._memmap_data(info) for info in (
            self.states_info, self.rewards_info, self.visit_counts_info)]

        return [torch.tensor(m[idx]) for m in memmaps]


class AugmentedExperienceChunk(ExperienceChunk):
    """Experience data augmented with dihedral rotations and reflections.

    Note that this depends on the structure of the policy output, e.g., whether it includes a pass move.
    """
    def __init__(self, *args):
        super().__init__(*args)
        self.num_original = super().__len__()

    def __len__(self):
        return 8 * self.num_original

    def __getitem__(self, idx):
        transform = Dihedral(idx // self.num_original)
        # print('selected transform:', idx // self.num_original)

        base_idx = idx % self.num_original
        # print('base idx:', base_idx)
        state, reward, visit_counts = super().__getitem__(base_idx)
        state = transform.forward(state)

        # Visit counts has shape [board_size^2 + 1]
        board_size = state.shape[1]
        # print('board size:', board_size)
        board_visit_counts = visit_counts[:-1].reshape(1, board_size, board_size)
        board_visit_counts = transform.forward(board_visit_counts)
        visit_counts = torch.cat((board_visit_counts.reshape(board_size * board_size), visit_counts[-1:]))
        assert torch.numel(visit_counts) == board_size * board_size + 1

        return state, reward, visit_counts



if __name__ == '__main__':
    # data = ExperienceChunk(*[f'example/{k}_1.json' for k in ('states', 'rewards', 'visit_counts')])
    # data_aug = AugmentedExperienceChunk(*[f'example/{k}_1.json' for k in ('states', 'rewards', 'visit_counts')])
    data = ExperienceSet('example')
    # data = ExperienceSubset('example', 5)
