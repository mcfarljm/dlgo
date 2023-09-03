#include "agent_naive.h"
#include "myrand.h"

Move RandomBot::select_move(const GameState& game_state) {
  std::vector<Point> candidates;
  for (auto r=1; r <= game_state.board->num_rows; r++) {
    for (auto c=1; c <= game_state.board->num_cols; c++) {
      auto candidate = Point(r, c);
      if (game_state.is_valid_move(Move::play(candidate)) &&
          ! is_point_an_eye(*game_state.board, candidate, game_state.next_player))
        candidates.push_back(candidate);
    }
  }
  if (candidates.empty())
    return Move::pass();

  std::uniform_int_distribution<> dist(0, candidates.size() - 1);
  auto idx = dist(rng);
  return Move::play(candidates[idx]);
}


void FastRandomBot::update_cache(std::pair<int, int> dim) {
  cached_dim = dim;
  point_cache.clear();
  for (auto r=1; r<= dim.first; ++r) {
    for (auto c=1; c<= dim.second; ++c) {
      point_cache.push_back(Point(r, c));
    }
  }
}


Move FastRandomBot::select_move(const GameState& game_state) {
  // Why doesn't make pair work?
  // auto dim = std::make_pair<int, int>(game_state.board->num_rows, game_state.board->num_cols);
  std::pair<int,int> dim = {game_state.board->num_rows, game_state.board->num_cols};
  if (dim != cached_dim)
    update_cache(dim);

  assert(! point_cache.empty());
  std::vector<size_t> point_indices;
  for (size_t i=0; i<point_cache.size(); ++i)
    point_indices.push_back(i);
  std::shuffle(point_indices.begin(), point_indices.end(), rng);
  for (auto i : point_indices) {
    auto p = point_cache[i];
    if (game_state.is_valid_move(Move::play(p)) &&
        ! is_point_an_eye(*game_state.board, p, game_state.next_player))
      return Move::play(p);
  }
  return Move::pass();
}
