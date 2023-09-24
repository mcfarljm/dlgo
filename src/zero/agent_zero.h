#ifndef AGENT_ZERO_H
#define AGENT_ZERO_H

#include <memory>
#include <optional>
#include <torch/script.h> // One-stop header.

#include "encoder.h"
#include "experience.h"
#include "../agent_base.h"

class Branch {
public:
  float prior;
  int visit_count = 0;
  float total_value = 0.0;

public:
  Branch(float prior) : prior(prior) {}
};

class ZeroNode {
public:
  ConstGameStatePtr game_state;
  std::weak_ptr<ZeroNode> parent;
  std::optional<Move> last_move;
  std::unordered_map<Move, std::shared_ptr<ZeroNode>, MoveHash> children;
  std::unordered_map<Move, Branch, MoveHash> branches;
  float value;
  int total_visit_count = 1;
  bool terminal;

  ZeroNode(ConstGameStatePtr game_state, float value,
           std::unordered_map<Move, float, MoveHash> priors,
           std::weak_ptr<ZeroNode> parent = std::weak_ptr<ZeroNode>(),
           std::optional<Move> last_move = std::nullopt);

  void add_child(Move move, std::shared_ptr<ZeroNode> child) {
    children.emplace(move, child);
  }

  // More efficient to work with the children.find iterator directly if
  // possible, to avoid repeated calls.

  // bool has_child(Move move) {
  //   return children.find(move) != children.end();
  // }

  // std::shared_ptr<ZeroNode> get_child(Move move) {
  //   return children.find(move)->second;
  // }

  void record_visit(Move m, float val);

  float expected_value(Move m);

  float prior(Move m) {
    return branches.find(m)->second.prior;
  }

  int visit_count(Move m) {
    auto it = branches.find(m);
    if (it != branches.end())
      return it->second.visit_count;
    return 0;
  }
};

class ZeroAgent : public Agent {
  torch::jit::script::Module model;
  std::shared_ptr<Encoder> encoder;
  int num_rounds;
  float c_uct;

  std::shared_ptr<ExperienceCollector> collector;

  // If True, always select moves that maximize visit count.  Otherwise, initial
  // moves are selected in proportion to visit count.
  bool greedy;
  constexpr static int GREEDY_MOVE_THRESHOLD = 30;

  // Concentration parameter for dirichlet noise:
  constexpr static double DIRICHLET_CONCENTRATION = 0.03;
  constexpr static float DIRICHLET_WEIGHT = 0.25;

public:
  ZeroAgent(torch::jit::script::Module model,
            std::shared_ptr<Encoder> encoder,
            int num_rounds,
            bool greedy = true,
            float c_uct = 1.5) :
    model(model), encoder(encoder), num_rounds(num_rounds), c_uct(c_uct), greedy(greedy) {}

  Move select_move(const GameState&);

  void set_collector(std::shared_ptr<ExperienceCollector> c) {
    collector = c;
  }

private:
  std::shared_ptr<ZeroNode> create_node(ConstGameStatePtr game_state,
                                        std::optional<Move> move = std::nullopt,
                                        std::weak_ptr<ZeroNode> parent = std::weak_ptr<ZeroNode>());
  Move select_branch(std::shared_ptr<ZeroNode> node);
};

#endif // AGENT_ZERO_H
