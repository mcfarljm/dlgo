# DLGo

DLGo is a small project that was used to test and investigate deep reinforcement learning for the game of Go.  It implements an [AlphaZero](https://arxiv.org/abs/1712.01815)-style model that combines Monte Carlo Tree Search (MCTS) with a neural network for policy and value prediction.  The main training run was conducted for a 9x9 board and involved six iterations and approximately 6 days of training.  Using this neural network, the program easily defeats a complete beginner and seems to play at a strength similar to `gnugo --level 1`.

The design and implementation for board and rules handling largely followed the book [Deep Learning and the Game of Go](https://www.manning.com/books/deep-learning-and-the-game-of-go).

The code is written in C++.  The neural networks are defined in PyTorch and exported to TorchScript for evaluation in the C++ code.

## Features

* AlphaZero-style engine that combines MCTS with a multi-output neural network.
  * Dirichlet random noise added to move priors at the root node of each search.
  * Accommodates both greedy and proportional move selection based on visit counts.
  * To take advantage of symmetry, the board position is randomly rotated/flipped prior to each neural network evaluation.  Random symmetries are also used during training.
  * Monte Carlo Tree Search is done in serial, dynamic memory is used for tree expansion, and the search tree is reset for each move.
* Support for [Go Text Protocol](http://www.lysator.liu.se/~gunnar/gtp/) (GTP).
* Complete framework for self-play and training.  The [`run_training.sh`](scripts/run_training.sh) Bash script is provided as an example for fully-automated and parallelized self-play and training updates.
* In addition to the AlphaZero deep learning agent, rudimentary versions of pure MCTS and alpha-beta search are also included.

## Usage

CMake and LibTorch (the C++ distribution of PyTorch) are required to build and run the engine using an existing neural network file.  PyTorch is required for training.

* `mkdir build; cd build; cmake .. -DCMAKE_PREFIX_PATH=<path-to-libtorch> -DCMAKE_BUILD_TYPE=RELEASE; make`
* Run tests using `ctest`
* See usage information for the GTP driver: `./dlgobot -h`
* See usage information for the self-play driver: `./zero_sim -h`
* To run self-play training iterations, see the [`run_training.sh`](scripts/run_training.sh) example script, which provides a starting point.

## Design

The neural network is implemented using PyTorch.  During training, two versions of the network data are retained: (1) the network weights for use in subsequent training updates, and (2) a TorchScript export for use in the C++ code.

The experience data generated during self-play are stored in a simple raw binary format with accompanying metadata in json.  This makes it possible to efficiently load and take subsets of the data in Python during training using [`numpy.memmap`](https://numpy.org/doc/stable/reference/generated/numpy.memmap.html).

For each game, the experience data include:

* A tensor describing the input state before each move.
* A tensor describing the MCTS visit counts associated with each move (used as the target for the policy network).
* A tensor describing the reward (game outcome) associated with each move.

## Details of the main training run

Following are details for the main training run done for a 9x9 board.

### Input encoder

The main run uses a simple 11-plane feature encoder, based on [Deep Learning and the Game of Go](https://www.manning.com/books/deep-learning-and-the-game-of-go):

* Planes 0-3: current player's stones with 1, 2, 3, and 4+ liberties
* Planes 4-7: Opponent's stones with 1, 2, 3, and 4+ liberties
* Plane 8: 1 if white is to move (current player gets komi)
* Plane 9: 1 if black is to move (opponent gets komi)
* Plane 10: Move would be illegal due to ko

### Network architecture

The neural network architecture used for the main run is defined in `nn/nine/conv_4x64.py`.  It is a miniature, simplified version of the architecture used by [AlphaGoZero](https://www.deepmind.com/publications/mastering-the-game-of-go-without-human-knowledge).

The input to the network is a $9 \times 9 \times 11$ image stack comprising 11 feature planes as outlined in the previous section.

The input features are processed by a tower of 4 convolutional blocks with 64 filters, batch normalization, and ReLU activation.  The output of this tower than feeds two separate "heads" for computing the policy and value.

The policy head includes a convolution with 2 filters, followed by a fully-connected linear layer and softmax that outputs policy probabilities for all moves (including pass).

The value head includes a convolution with 1 filter, followed by a fully-connected linear layer with 256 outputs and ReLU activation, followed by a linear layer outputing a scalar, and finally a tanh mapping to the range +/-1.

### Algorithm settings

Move selection:

* 800 MCTS rounds are used during self-play and model evaluation.
* Dirichlet noise is added to the root node with a concentration parameter equal to $0.03 \times 19 \times 19 / n_{moves}$, following [Katago](https://arxiv.org/abs/1902.10565).
* During self-play, the first 14 moves ($30 \times 9 / 19$) are selected randomly in proportion to visit counts, and remaining moves are selected greedily based on maximum visit count.
* Tree search is done in serial.  During self-play, parallelization is accomplished by running multiple processes.

Self-play:

* Self-play games were played to completion with no resignation or move limit.
* Each iteration consisted of 4,000 self-play games, after which a training update was performed.

Training:

* Training batch size was 256 moves.
* For each training iteration, all 8 symmetries for each position were created, and 1/8 of the total positions were selected randomly for training.  For example, supposing 100 moves per game, we have $100 \times 4,000 \times 8 = 3,200,000$ positions available, and 400,000 are randomly selected for training.  With a batch size of 256, this corresponds to roughly 1,500 training steps.
* Stochastic gradient descent was used with a learning rate of 5e-3.

### Computational setup

The main training run was done on an Amazon EC2 instance with 8 virtual CPUs.  To obtain the best self-play throughput with the serial MCTS implementation, 8 self-play processes were run concurrently, and neural network model evaluation for each process was configured to use one thread.  Typical throughput was approximately 0.74 moves per second per process, for a total of approximately 5.9 moves per second (profiling suggests that this is dominated by neural network evaluation time, with almost no appreciable overhead from the MCTS code).  A typical self-play iteration consisting of 500 games per worker (4,000 total) took about 20 hours.  After generating self-play data, the training update was fast, taking on the order of minutes.

### Results

Each iteration of the neural network was evaluated against the previous version for 200 games (each agent plays 100 games as black and white).  During the evaluation games, 800 MCTS rounds were used, Dirichlet noise was included in the root node, and move selection was done greedily based on node visit count.

The networks produced by the first two training iterations did not lose any matches to prior versions.  Subsequent win rates continued to drop, reaching about 78% for the sixth iteration.  Approximate relative ELO computed using the Bayeselo program are given below.  These results are based only on the evaluations that each agent played against the previous version.  In the naming scheme, "v2" indicates the base version associated with the main run, "v2.0" denotes the randomly initialized neural network, "v2.1" is the network after one training iteration, etc.

```
Rank Name   Elo    +    - games score oppo. draws 
   1 v2.6  1200   57   57   200   78%   984    0% 
   2 v2.5   984   43   43   400   54%   944    0% 
   3 v2.4   688   48   48   400   51%   677    0% 
   4 v2.3   370   60   60   400   55%   252    0% 
   5 v2.2  -183  116  116   400   52%  -367    0% 
   6 v2.1 -1104  216  216   400   50% -1069    0% 
   7 v2.0 -1955  279  279   200    0% -1104    0% 
```

