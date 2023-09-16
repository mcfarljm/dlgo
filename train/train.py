import torch
from torch.utils.data import DataLoader

from data import ExperienceSubset


def cross_entropy_loss_fn(policies, visit_counts):
    search_probs = visit_counts / visit_counts.sum(1, keepdims=True)
    assert search_probs[0].sum() > 0.999 and search_probs[0].sum() < 1.001

    return -(search_probs * torch.log(policies)).sum()


def train(dataloader, model, optimizer):
    # The papers are a little bit unclear, mentioning MSE but showing SSE in the
    # equation.  From what I can tell, SSE seems much more comparable to
    # cross-entropy loss.
    sse_loss_fn = torch.nn.MSELoss(reduction='sum')
    size = len(dataloader.dataset)
    model.train()
    for batch_num, (states, rewards, visit_counts) in enumerate(dataloader):

        # Compute prediction error
        policies, values = model(states)

        sse_loss = sse_loss_fn(values.squeeze(), rewards)
        cross_entropy_loss = cross_entropy_loss_fn(policies, visit_counts)

        loss = sse_loss + cross_entropy_loss

        # Backpropagation
        loss.backward()
        optimizer.step()
        optimizer.zero_grad()

        if batch_num % 2 == 0:
            loss, current = loss.item(), (batch_num + 1) * len(states)
            sse_loss, ce_loss = sse_loss.item(), cross_entropy_loss.item()
            print(f"loss: {loss:>7f} ({sse_loss:>3f}, {ce_loss:>3f})  [{current:>5d}/{size:>5d}]")


if __name__ == '__main__':
    import sys
    sys.path.append('../nn/nine')
    from conv_4x64 import GoNet
    
    grid_size = 9
    encoder_channels = 11
    model = GoNet(in_channels=encoder_channels, grid_size=grid_size)
    model.load_state_dict(torch.load('../nn/nine/conv_4x64.pt'))
    
    dataset = ExperienceSubset('example', 6000)
    dataloader = DataLoader(dataset, 256)

    optimizer = torch.optim.SGD(model.parameters(),
                                # Paper starts wtih 1e-2, but seems too large
                                lr=1e-3,
                                momentum=0.9,
                                weight_decay=1e-4)

    train(dataloader, model, optimizer)
