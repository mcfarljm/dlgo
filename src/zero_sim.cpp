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

  cxxopts::Options options("zero_sim", "Simulate games using zero agent");

  options.add_options()
    ("network", "Path to pytorch script file", cxxopts::value<std::string>())
    ("o,output-path", "Directory to store output", cxxopts::value<std::string>())
    ("l,label", "Label to use within experience directory", cxxopts::value<std::string>()->default_value(""))
    ("r,rounds", "Number of rounds", cxxopts::value<int>()->default_value("800"))
    ("g,num-games", "Number of games", cxxopts::value<int>()->default_value("1"))
    ("e,save-every", "Interval at which to save experience", cxxopts::value<int>()->default_value("100"))
    ("b,board-size", "Board size", cxxopts::value<int>()->default_value("9"))
    ("v,verbosity", "Verbosity level", cxxopts::value<int>()->default_value("0"))
    ("t,num-threads", "Number of pytorch threads", cxxopts::value<int>())
    ("h,help", "Print usage")
    ;

  options.parse_positional({"network"});
  options.positional_help("<network_file>");

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

  if (! args.count("network")) {
    std::cout << options.help() << std::endl;
    exit(1);
  }

  std::string output_path;
  bool store_experience = false;

  auto num_rounds = args["rounds"].as<int>();
  auto num_games = args["num-games"].as<int>();
  auto save_interval = args["save-every"].as<int>();
  auto board_size = args["board-size"].as<int>();
  auto verbosity = args["verbosity"].as<int>();
  if (args.count("output-path")) {
    output_path = args["output-path"].as<std::string>();
    store_experience = true;
  }
  std::string experience_label(args["label"].as<std::string>());
  if (experience_label.size()) {
    if (! store_experience) {
      std::cerr << "Error, experience label passed without output path" << std::endl;
      exit(1);
    }
    experience_label.insert(0, "_");
  }

  if (store_experience) {
    if (std::filesystem::exists(output_path) && ! std::filesystem::is_directory(output_path)) {
      std::cerr << "output path exists and is not a directory: " + output_path << std::endl;
      exit(1);
    }
  }
    
  if (args.count("num-threads")) {
    std::cout << "setting " << args["num-threads"].as<int>() << " pytorch threads" << std::endl;
    at::set_num_threads(args["num-threads"].as<int>());
  }

  c10::InferenceMode guard;
  torch::jit::script::Module model;
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    model = torch::jit::load(args["network"].as<std::string>());
  }
  catch (const c10::Error& e) {
    std::cerr << "error loading the model\n";
    return -1;
  }

  std::cout << "Model loaded\n";

  auto encoder = std::make_shared<SimpleEncoder>(board_size);

  auto black_collector = std::make_shared<ExperienceCollector>();
  auto white_collector = std::make_shared<ExperienceCollector>();

  auto black_agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, false);
  auto white_agent = std::make_unique<ZeroAgent>(model, encoder, num_rounds, false);

  black_agent->set_collector(black_collector);
  white_agent->set_collector(white_collector);

  int num_black_wins = 0;
  int save_counter = 0;
  int total_num_moves = 0;
  auto cumulative_timer = Timer();
  for (int game_num=0; game_num < num_games; ++game_num) {
    auto timer = Timer();
    auto [winner, num_moves] = simulate_game(board_size, black_agent.get(), white_agent.get(), verbosity);
    auto duration = timer.elapsed();
    total_num_moves += num_moves;
    if (num_games <= 5) {
      std::cout << "Game: " << num_moves << " moves in " << duration;
      std:: cout << " s (" << num_moves / duration << " mv/s, " << duration / num_moves << " s/mv)" << std::endl;
    }

    auto total_duration = cumulative_timer.elapsed();
    auto games_per_sec = (game_num + 1) / total_duration;
    auto remaining_sec = (num_games - game_num - 1) / games_per_sec;
    std::cout << num_black_wins << "/" << game_num + 1 << "/" << num_games;
    std::cout << std::fixed << std::setprecision(1) << " (" << 100.0 * num_black_wins / (game_num + 1) << "% Blk)";
    std::cout << "  [" << format_seconds(total_duration) << " < " << format_seconds(remaining_sec) << "]" << std::endl;

    auto black_reward = winner == Player::black ? 1.0 : -1.0;
    if (winner == Player::black) ++num_black_wins;
    black_collector->complete_episode(black_reward);
    white_collector->complete_episode(-1.0 * black_reward);

    if (store_experience && (game_num + 1) % save_interval == 0) {
      black_collector->append(*white_collector);
      black_collector->serialize_binary(output_path, experience_label + "_" + std::to_string(save_counter));
      black_collector->reset();
      white_collector->reset();
      ++save_counter;
    }
  }

  std::cout << "Finished: " << total_num_moves << " moves at " << std::setprecision(2) << total_num_moves / cumulative_timer.elapsed() << " moves / second" << std::endl;

  if (store_experience) {
    black_collector->append(*white_collector);
    black_collector->serialize_binary(output_path, experience_label);
  }
}
