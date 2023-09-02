#include "goboard.h"
#include <iostream>
#include <string>


static constexpr char COLS[] = "ABCDEFGHJKLMNOPQRST";


static std::string stone_to_char(std::optional<Player> stone) {
  if (! stone)
    return " . ";
  else if (stone.value() == Player::white)
    return " o ";
  else
    return " x ";
}


GoString* GoString::merged_with(GoString &go_string) const {
  // std::cout << "in merged_with for string with " << go_string.stones.size() << " stones\n";
  assert(go_string.color == color);
  // std::cout << "combining stones\n";
  PointSet combined_stones(stones);
  combined_stones.insert(go_string.stones.begin(), go_string.stones.end());
  // std::cout << "combining liberties\n";
  PointSet new_liberties(liberties);
  new_liberties.insert(go_string.liberties.begin(), go_string.liberties.end());
  // std::cout << "removing stones from liberties\n";
  for (const auto& stone : combined_stones)
    new_liberties.erase(stone);
  // std::cout << "returning new pointer\n";
  return new GoString(color, combined_stones, new_liberties);
}


std::ostream& operator<<(std::ostream& os, const Board& b) {
  for (auto row = b.num_rows; row > 0; row--) {
    auto pad = row <= 9 ? " " : "";
    std::string line;
    for (auto col = 1; col <= b.num_cols; col++) {
      auto stone = b.get(Point(row, col));
      line += stone_to_char(stone);
    }
    os << pad << row << " " << line << std::endl;
  }
  os << "    ";
  for (auto c=0; c< b.num_cols; c++) {
    os << COLS[c] << "  ";
  }
  os << std::endl;
  return os;
}

void Board::place_stone(Player player, const Point& point) {
  assert(is_on_grid(point));
  assert(grid.find(point) == grid.end());

  std::unordered_set<std::shared_ptr<GoString>> adjacent_same_color;
  std::unordered_set<std::shared_ptr<GoString>> adjacent_opposite_color;
  PointSet liberties;

  // Check neighbors
  // std::cout << "Looping over neighbors\n";
  for (const auto &neighbor : point.neighbors()) {
    if (! is_on_grid(neighbor)) continue;
    auto neighbor_string_it = grid.find(neighbor);
    if (neighbor_string_it == grid.end()) {
      // std::cout << "Adding liberty\n";
      liberties.insert(neighbor);
    }
    else if (neighbor_string_it->second->color == player) {
      // std::cout << "Found adjacent same color\n";
      adjacent_same_color.insert(neighbor_string_it->second);
    }
    else {
      // std::cout << "Found adjacent opposite color\n";
      adjacent_opposite_color.insert(neighbor_string_it->second);
    }
  }

  PointSet single_point({point});
  auto new_string = std::make_shared<GoString>(player, std::move(single_point), liberties);
  // Merge new string with adjacent ones:
  // std::cout << "Merging new string with " << adjacent_same_color.size() << " adjacent\n";
  for (const auto &same_color_string : adjacent_same_color) {
    // std::cout << "merging new string with same color with " << same_color_string->stones.size() << " stones\n";
    new_string.reset(new_string->merged_with(*same_color_string));
  }
  for (const auto &new_string_point : new_string->stones)
    grid[new_string_point] = new_string;

  hash ^= hasher.point_keys[size_t(player)][point.row-1][point.col-1];
  // std::cout << "  Hash place << " << int(player) << " " << point.row << " " << point.col << " " << hasher.point_keys[size_t(player)][point.row-1][point.col-1] << " " << hash << std::endl;

  for (const auto &other_color_string : adjacent_opposite_color)
    other_color_string->remove_liberty(point);
  for (const auto &other_color_string : adjacent_opposite_color)
    if (other_color_string->num_liberties() == 0)
      remove_string(other_color_string);
}


void Board::remove_string(std::shared_ptr<GoString> string) {
  for (const auto& point : string->stones) {
    for (const auto& neighbor : point.neighbors()) {
      auto neighbor_string_it = grid.find(neighbor);
      if (neighbor_string_it == grid.end())
        continue;
      else if (neighbor_string_it->second != string)
        neighbor_string_it->second->add_liberty(point);
    }
    grid.erase(point);
    hash ^= hasher.point_keys[size_t(string->color)][point.row-1][point.col-1];
    // std::cout << "  Hash remove << " << int(string->color) << " " << point.row << " " << point.col << " " << hasher.point_keys[size_t(string->color)][point.row-1][point.col-1] << " " << hash << std::endl;
  }
}


GridMap deepcopy_grid(const GridMap& grid) {
  std::unordered_map<GoString*, std::shared_ptr<GoString>> memo;
  for (const auto&[point, sptr] : grid) {
    auto id = sptr.get();
    if (memo.find(id) == memo.end())
      // Might investigte the best way to do this:
      // memo[id] = std::shared_ptr<GoString>(new GoString(*sptr));
      memo.try_emplace(id, new GoString(*sptr));
  }
  GridMap new_grid;
  for (const auto&[point, sptr] : grid)
    new_grid.emplace(point, memo.find(sptr.get())->second);
  return new_grid;
}

GameStatePtr GameState::apply_move(Move m) {
  // std::cout << "In apply move " << m.is_play << "\n";
  BoardPtr next_board;
  if (m.is_play) {
    next_board = board->deepcopy();
    next_board->place_stone(next_player, m.point.value());
  } else {
    next_board.reset(new Board(*board)); // Shallow copy
  }
  return std::make_shared<GameState>(next_board, other_player(next_player), shared_from_this(), m);
}


bool GameState::is_over() const {
  if (! last_move)
    return false;
  else if (last_move.value().is_resign)
    return true;
  auto second_last_move = previous_state->last_move;
  if (! second_last_move)
    return false;
  else
    return last_move.value().is_pass && second_last_move.value().is_pass;
}

bool GameState::is_move_self_capture(Player player, Move m) const {
  if (! m.is_play)
    return false;
  auto next_board = board->deepcopy();
  next_board->place_stone(player, m.point.value());
  auto new_string = next_board->get_go_string(m.point.value());
  assert(new_string);
  return new_string.value()->num_liberties() == 0;
}

bool GameState::does_move_violate_ko(Player player, Move m) const {
  if (! m.is_play)
    return false;
  auto next_board = board->deepcopy();
  next_board->place_stone(player, m.point.value());
  auto next_situation = std::make_pair(other_player(player), next_board->get_hash());
  // std::cout << "Checking ko, next: " << int(other_player(player)) << " " << next_board->get_hash() << std::endl;
  // for (const auto& [p, h] : previous_hashes) {
  //   std::cout << "   prev:  " << int(p) << " " << h << std::endl;
  // }
  return std::find(previous_hashes.begin(), previous_hashes.end(), next_situation) != previous_hashes.end();
}

bool GameState::is_valid_move(Move m) const {
  if (is_over())
    return false;
  if (m.is_pass || m.is_resign)
    return true;
  return (! board->get(m.point.value())) &&
    (! is_move_self_capture(next_player, m)) &&
    (! does_move_violate_ko(next_player, m));
}
