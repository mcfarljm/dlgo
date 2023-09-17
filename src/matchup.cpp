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
#include "agent_naive.h"


std::unique_ptr<Agent> load_zero_agent(const std::string network_path,
                                       int board_size,
                                       int num_rounds) {
  c10::InferenceMode guard;
  torch::jit::script::Module model;
  try {
    model = torch::jit::load(network_path);
  }
  catch (const c10::Error& e) {
    std::cerr << "Error loading model: " << network_path << std::endl;
    // Return emptpy pointer
    return std::unique_ptr<Agent>();
  }

  std::cout << "Loaded: " << network_path << std::endl;

  auto encoder = std::make_shared<SimpleEncoder>(board_size);
  auto agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, true);
  return agent;
}


std::unique_ptr<Agent> load_agent(const std::string identifier,
                                       int board_size,
                                       int num_rounds) {
  if (identifier == "random") {
    std::cout << "loading random agent" << std::endl;
    return std::make_unique<FastRandomBot>();
  }
  else
    return load_zero_agent(identifier, board_size, num_rounds);
}


int main(int argc, const char* argv[]) {

  cxxopts::Options options("matchup", "Pair two agents against each other");

  options.add_options()
    ("agent1", "Netowrk path or 'random'", cxxopts::value<std::string>())
    ("agent2", "Netowrk path or 'random'", cxxopts::value<std::string>())
    ("r,rounds", "Number of rounds", cxxopts::value<int>()->default_value("800"))
    ("g,num-games", "Number of games", cxxopts::value<int>()->default_value("1"))
    ("b,board-size", "Board size", cxxopts::value<int>()->default_value("9"))
    ("v,verbosity", "Verbosity level", cxxopts::value<int>()->default_value("0"))
    ("t,num-threads", "Number of pytorch threads", cxxopts::value<int>())
    ("h,help", "Print usage")
    ;

  options.parse_positional({"agent1", "agent2"});
  options.positional_help("<agent1> <agent2>");

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

  if (! args.count("agent1") || ! args.count("agent2")) {
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

  auto agent1 = load_agent(args["agent1"].as<std::string>(),
                           board_size, num_rounds);
  auto agent2 = load_agent(args["agent2"].as<std::string>(),
                           board_size, num_rounds);
  if (! agent1 || ! agent2)
    return -1;


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
    std::cout << std::fixed << std::setprecision(1);
    std::cout << " (" << 100.0 * agent1_wins / (game_num + 1) << "%";
    std::cout << ", " << 100.0 * agent1_wins_as_black / agent1_num_black_games << "% as Blk)";
    std::cout << ", " << total_num_moves / (game_num + 1) << " mpg";
    std::cout << std::defaultfloat << std::setprecision(4);
    std::cout << ", " << total_num_moves / total_duration << " mps";
    std::cout << "  [" << format_seconds(total_duration) << " < " << format_seconds(remaining_sec) << "]" << std::endl;

  }

}
