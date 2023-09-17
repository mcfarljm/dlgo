#ifndef SIMULATION_H
#define SIMULATION_H

#include <utility>

#include "agent_base.h"


/// Simulate game and return (winner, num_moves)
std::pair<Player, int> simulate_game(int board_size,
                                     Agent* black_agent,
                                     Agent* white_agent,
                                     int verbosity = 0,
                                     int max_moves = 10000);


#endif // SIMULATION_H
