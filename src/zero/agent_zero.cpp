#include "agent_zero.h"


ZeroNode::ZeroNode(ConstGameStatePtr game_state, float value,
                   std::unordered_map<Move, float, MoveHash> priors,
                   std::weak_ptr<ZeroNode> parent,
                   std::optional<Move> last_move) :
  game_state(game_state), value(value), parent(parent), last_move(last_move),
  terminal(game_state->is_over()) {

  for (const auto &[move, p] : priors) {
    if (game_state->is_valid_move(move))
      branches.emplace(move, p);
  }

  if (terminal)
    // Override the model's value estimate with actual result
    value = (game_state->next_player == game_state->winner().value()) ? 1.0 : -1.0;
}


void ZeroNode::record_visit(std::optional<Move> move, float value) {
  ++total_visit_count;
  if (move) {
    auto it = branches.find(move.value());
    assert(it != branches.end());
    (it->second.visit_count)++;
    (it->second.total_value) += value;
  }
}


float ZeroNode::expected_value(Move m) {
  auto branch = branches.find(m)->second;
  if (branch.visit_count == 0)
    return 0.0;
  return branch.total_value / branch.visit_count;
}
