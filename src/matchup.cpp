#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <string>
#include <array>
#include <filesystem>

#include <cxxopts.hpp>

#include "goboard.h"
#include "zero/agent_zero.h"
#include "utils.h"
#include "scoring.h"
#include "simulation.h"


int main(int argc, const char* argv[]) {

  cxxopts::Options options("matchup", "Pair two agents against each other");

  options.add_options()
    ("network1", "Path to pytorch script file", cxxopts::value<std::string>())
    ("network2", "Path to pytorch script file", cxxopts::value<std::string>())
    ("r,rounds", "Number of rounds", cxxopts::value<int>()->default_value("800"))
    ("g,num-games", "Number of games", cxxopts::value<int>()->default_value("1"))
    ("b,board-size", "Board size", cxxopts::value<int>()->default_value("9"))
    ("v,verbosity", "Verbosity level", cxxopts::value<int>()->default_value("0"))
    ("t,num-threads", "Number of pytorch threads", cxxopts::value<int>())
    ("h,help", "Print usage")
    ;

  options.parse_positional({"network1", "network2"});
  options.positional_help("<network_file1> <network_file2>");

  cxxopts::ParseResult args;
  try {
    args = options.parse(argc, argv);
  }
  catch (const cxxopts::exceptions::exception& e) {
    std::cout << options.help() << std::endl;
    exit(1);
  }

  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  if (! args.count("network1") || ! args.count("network2")) {
    std::cout << options.help() << std::endl;
    exit(1);
  }

  auto num_rounds = args["rounds"].as<int>();
  auto num_games = args["num-games"].as<int>();
  auto board_size = args["board-size"].as<int>();
  auto verbosity = args["verbosity"].as<int>();
    
  if (args.count("num-threads")) {
    std::cout << "setting " << args["num-threads"].as<int>() << " pytorch threads" << std::endl;
    at::set_num_threads(args["num-threads"].as<int>());
  }

  c10::InferenceMode guard;
  torch::jit::script::Module model1;
  torch::jit::script::Module model2;
  try {
    model1 = torch::jit::load(args["network1"].as<std::string>());
    model2 = torch::jit::load(args["network2"].as<std::string>());
  }
  catch (const c10::Error& e) {
    std::cerr << "error loading the model\n";
    return -1;
  }

  std::cout << "Models loaded\n";

  auto encoder = std::make_shared<SimpleEncoder>(board_size);

  auto agent1 = std::make_unique<ZeroAgent>(model1, encoder, num_rounds, true);
  auto agent2 = std::make_unique<ZeroAgent>(model2, encoder, num_rounds, true);
  Agent* black_agent;
  Agent* white_agent;

  int agent1_wins = 0;
  int agent1_wins_as_black = 0;
  int agent1_num_black_games = 0;
  int total_num_moves = 0;
  auto cumulative_timer = Timer();
  for (int game_num=0; game_num < num_games; ++game_num) {
    if (game_num % 2 == 0) {
      // Agent 1 plays black on even games
      black_agent = agent1.get();
      white_agent = agent2.get();
      ++agent1_num_black_games;
    } else {
      black_agent = agent2.get();
      white_agent = agent1.get();
    }
    auto timer = Timer();
    auto [winner, num_moves] = simulate_game(board_size, black_agent, white_agent, verbosity);
    auto duration = timer.elapsed();
    total_num_moves += num_moves;
    if (num_games <= 5) {
      std::cout << "Game: " << num_moves << " moves in " << duration;
      std:: cout << " s (" << num_moves / duration << " mv/s, " << duration / num_moves << " s/mv)" << std::endl;
    }

    if (game_num % 2 == 0) {
      // Agent 1 plays black
      if (winner == Player::black) {
        ++agent1_wins;
        ++agent1_wins_as_black;
      }
    }
    else {
      // Agent 1 plays white
      if (winner == Player::white) {
        ++agent1_wins;
      }
    }

    auto total_duration = cumulative_timer.elapsed();
    auto games_per_sec = (game_num + 1) / total_duration;
    auto remaining_sec = (num_games - game_num - 1) / games_per_sec;
    std::cout << agent1_wins << "/" << game_num + 1 << "/" << num_games;
    std::cout << std::fixed << std::setprecision(1) << " (" << 100.0 * agent1_wins / (game_num + 1) << "%";
    std::cout << ", " << 100.0 * agent1_wins_as_black / agent1_num_black_games << "% as Blk)";
    std::cout << "  [" << format_seconds(total_duration) << " < " << format_seconds(remaining_sec) << "]" << std::endl;

  }

  std::cout << "Finished: " << total_num_moves << " moves at " << std::setprecision(2) << total_num_moves / cumulative_timer.elapsed() << " moves / second" << std::endl;

}
