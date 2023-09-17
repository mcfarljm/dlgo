import os
import time

import torch
from torch import nn
import click

device = 'cpu'


class GoNet(nn.Module):
    def __init__(self, in_channels=2, grid_size=9, policy_size=82):
        super().__init__()
        self.pb = nn.Sequential(
            nn.Conv2d(in_channels, 64, 3, padding='same'),
            nn.BatchNorm2d(64),
            nn.ReLU(),

            nn.Conv2d(64, 64, 3, padding='same'),
            nn.BatchNorm2d(64),
            nn.ReLU(),

            nn.Conv2d(64, 64, 3, padding='same'),
            nn.BatchNorm2d(64),
            nn.ReLU(),

            nn.Conv2d(64, 64, 3, padding='same'),
            nn.BatchNorm2d(64),
            nn.ReLU(),
        )

        self.policy_stack = nn.Sequential(
            nn.Conv2d(64, 2, 1),
            nn.BatchNorm2d(2),
            nn.ReLU(),

            nn.Flatten(),
            nn.Linear(2*grid_size**2, policy_size),
            nn.Softmax(1),
        )

        self.value_stack = nn.Sequential(
            nn.Conv2d(64, 1, 1),
            nn.BatchNorm2d(1),
            nn.ReLU(),

            nn.Flatten(),
            nn.Linear(grid_size**2, 256),
            nn.ReLU(),

            nn.Linear(256, 1),
            nn.Tanh(),
        )

    def forward(self, x):
         pb = self.pb(x)
         policy = self.policy_stack(pb)
         value = self.value_stack(pb)
         return policy, value


def count_parameters(model):
    return sum(p.numel() for p in model.parameters() if p.requires_grad) 


# with torch.no_grad():
#     print('default threads:', torch.get_num_threads())
#     # torch.set_num_threads(1)
#     grid_size = 9
#     encoder_channels = 11
#     model = GoNet(in_channels=encoder_channels, grid_size=grid_size)
#     print('num params:', count_parameters(model))
#     model.eval()
#     n = 1000
#     X = torch.rand(n, encoder_channels, grid_size, grid_size, device=device)
#     print('shape:', X.shape)
#     tic = time.perf_counter()

#     # Individual calls:
#     for i in range(n):
#         (policy, value) = model(X[i:i+1,:,:,:])

#     # As a batch:
#     # (policy, value) = model(X)

#     toc = time.perf_counter()
#     print('shape:', policy.shape, value.shape)
#     print('delta:', toc-tic, (toc - tic) / n)


@click.command()
@click.option('-o', '--output', default='conv_4x64.pt')
@click.option('-f', '--force', is_flag=True, help='overwrite')
def main(output, force):
    if not force and (os.path.exists(output) or os.path.exists(output.replace('.pt', '.ts'))):
        raise ValueError('output exists')
    with torch.no_grad():
        grid_size = 9
        encoder_channels = 11
        model = GoNet(in_channels=encoder_channels, grid_size=grid_size)
        model.eval()

        torch.save(model.state_dict(), output)
        X = torch.rand(1, encoder_channels, grid_size, grid_size, device=device)
        traced_script_module = torch.jit.trace(model, X)
        traced_script_module.save(output.replace('.pt', '.ts'))


if __name__ == '__main__':
    main()
