#include <random>
#include "alphabeta.h"
#include "goboard.h"
#include "myrand.h"

namespace {
  constexpr int MAX_SCORE = 999999;
  constexpr int MIN_SCORE = -999999;
}

int alpha_beta_result(const GameState& game_state,
                      int max_depth, int best_black, int best_white,
                      int (*eval_fn)(const GameState&)) {
  if (game_state.is_over()) {
    if (game_state.winner() == game_state.next_player)
      return MAX_SCORE;
    else
      return MIN_SCORE;
  }

  if (max_depth == 0)
    return eval_fn(game_state);

  auto best_so_far = MIN_SCORE;
  for (const auto& candidate_move : game_state.legal_moves()) {
    auto next_state = game_state.apply_move(candidate_move);
    auto opponent_best_result = alpha_beta_result(*next_state,
                                                  max_depth - 1,
                                                  best_black,
                                                  best_white,
                                                  eval_fn);
    auto our_result = -1 * opponent_best_result;

    if (our_result > best_so_far)
      best_so_far = our_result;

    if (game_state.next_player == Player::white) {
      if (best_so_far > best_white)
        best_white = best_so_far;
      auto outcome_for_black = -1 * best_so_far;
      if (outcome_for_black < best_black)
        return best_so_far;
    } else { // black
      if (best_so_far > best_black)
        best_black = best_so_far;
      auto outcome_for_white = -1 * best_so_far;
      if (outcome_for_white < best_white)
        return best_so_far;
    }
  }
  
  return best_so_far;
}


Move AlphaBetaAgent::select_move(const GameState& game_state) {
  std::vector<Move> best_moves;
  int best_score = MIN_SCORE;
  int best_black = MIN_SCORE;
  int best_white = MIN_SCORE;
  // Loop over all legal moves:
  for (const auto& possible_move : game_state.legal_moves()) {
    if (possible_move.is_play)
      std::cout << "Searching move at " << possible_move.point.value() << std::endl;
    auto next_state = game_state.apply_move(possible_move);
    // Since our opponent plays next, figure out their best possible outcome
    // from there.
    auto opponent_best_outcome = alpha_beta_result(*next_state,
                                                   max_depth,
                                                   best_black,
                                                   best_white,
                                                   eval_fn);
    // Our outcome is the opposite of our opponent's outcome.
    auto our_best_outcome = -1 * opponent_best_outcome;
    if (best_moves.empty() || our_best_outcome > best_score) {
      // This is the best move so far.
      best_moves.clear();
      best_moves.push_back(possible_move);
      best_score = our_best_outcome;
      if (game_state.next_player == Player::black)
        best_black = best_score;
      else
        best_white = best_score;
    } else if (our_best_outcome == best_score) {
      // This is as good as our previous best move
      best_moves.push_back(possible_move);
    }
  }
  // For variety, randomly select among all equally good moves.
  std::uniform_int_distribution<> dist(0, best_moves.size() - 1);
  auto idx = dist(rng);
  return best_moves[idx];
}
