#ifndef AGENT_ZERO_H
#define AGENT_ZERO_H

#include <memory>
#include <optional>

#include "encoder.h"
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
  ConstGameStatePtr game_state;
  float value;
  std::weak_ptr<ZeroNode> parent;
  std::optional<Move> last_move;
  int total_visit_count = 1;
  std::unordered_map<Move, Branch, MoveHash> branches;
  std::unordered_map<Move, std::shared_ptr<ZeroNode>, MoveHash> children;
  bool terminal;

public:
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

  void record_visit(std::optional<Move> m, float val);

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

#endif // AGENT_ZERO_H
