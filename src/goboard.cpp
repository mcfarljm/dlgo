#include <iostream>
#include <string>
#include "goboard.h"
#include "scoring.h"


static constexpr char COLS[] = "ABCDEFGHJKLMNOPQRST";


// Initialize random hash keys:
const Hasher hasher {};


static std::string stone_to_char(std::optional<Player> stone) {
  if (! stone)
    return " . ";
  else if (stone.value() == Player::white)
    return " o ";
  else
    return " x ";
}


std::ostream& operator<<(std::ostream& os, const Move& m) {
  if (m.is_pass)
    os << "pass";
  else if (m.is_resign)
    os << "resign";
  else
    os << "play" << m.point.value();
  return os;
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


std::map<std::pair<int,int>,
         std::unordered_map<Point, std::vector<Point>, PointHash>> Board::neighbor_tables = {};

void Board::init_neighbor_table(std::pair<int,int> dim) {
  auto [rows, cols] = dim;
  std::cerr << "init neighbor table: " << rows << " " << cols << std::endl;
  std::unordered_map<Point, std::vector<Point>, PointHash> new_table;
  for (int r=1; r <= rows; ++r) {
    for (int c=1; c <= cols; ++c) {
      auto p = Point(r, c);
      std::vector<Point> true_neighbors;
      for (auto const& n : p.neighbors()) {
        if (1 <= n.row && n.row <= rows && 1 <= n.col && n.col <= cols)
          true_neighbors.push_back(n);
      }
      new_table.emplace(p, true_neighbors);
    }
  }
  Board::neighbor_tables.emplace(dim, new_table);
}


void Board::place_stone(Player player, const Point& point) {
  assert(is_on_grid(point));
  assert(grid.find(point) == grid.end());

  std::unordered_set<std::shared_ptr<GoString>> adjacent_same_color;
  std::unordered_set<std::shared_ptr<GoString>> adjacent_opposite_color;
  // PointSet liberties;
  std::vector<Point> liberties;

  // Check neighbors
  // std::cout << "Looping over neighbors\n";
  for (const auto &neighbor : neighbor_table_ptr->find(point)->second) {
    if (! is_on_grid(neighbor)) continue;
    auto neighbor_string_it = grid.find(neighbor);
    if (neighbor_string_it == grid.end()) {
      // std::cout << "Adding liberty\n";
      liberties.push_back(neighbor);
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

  auto new_string = std::make_shared<GoString>(player, FrozenPointSet({point}),
                                               FrozenSet<Point, PointHash>(liberties.begin(), liberties.end()));
  // Merge new string with adjacent ones:
  // std::cout << "Merging new string with " << adjacent_same_color.size() << " adjacent\n";
  for (const auto &same_color_string : adjacent_same_color) {
    // std::cout << "merging new string with same color with " << same_color_string->stones.size() << " stones\n";
    new_string = new_string->merged_with(*same_color_string);
  }
  for (const auto &new_string_point : new_string->stones)
    grid[new_string_point] = new_string;

  hash ^= hasher.point_keys[size_t(player)][point.row-1][point.col-1];
  // std::cout << "  Hash place << " << int(player) << " " << point.row << " " << point.col << " " << hasher.point_keys[size_t(player)][point.row-1][point.col-1] << " " << hash << std::endl;

  for (const auto &other_color_string : adjacent_opposite_color) {
    auto replacement = other_color_string->without_liberty(point);
    if (replacement->num_liberties() > 0)
      replace_string(other_color_string->without_liberty(point));
    else
      remove_string(other_color_string);
  }
  //   other_color_string->remove_liberty(point);
  // for (const auto &other_color_string : adjacent_opposite_color)
  //   if (other_color_string->num_liberties() == 0)
  //     remove_string(other_color_string);
}


void Board::replace_string(std::shared_ptr<GoString> new_string) {
  for (const auto& point : new_string->stones)
    grid[point] = new_string;
}


void Board::remove_string(std::shared_ptr<GoString> string) {
  for (const auto& point : string->stones) {
    for (const auto& neighbor : neighbor_table_ptr->find(point)->second) {
      auto neighbor_string_it = grid.find(neighbor);
      if (neighbor_string_it == grid.end())
        continue;
      else if (neighbor_string_it->second != string)
        replace_string(neighbor_string_it->second->with_liberty(point));
    }
    grid.erase(point);
    hash ^= hasher.point_keys[size_t(string->color)][point.row-1][point.col-1];
    // std::cout << "  Hash remove << " << int(string->color) << " " << point.row << " " << point.col << " " << hasher.point_keys[size_t(string->color)][point.row-1][point.col-1] << " " << hash << std::endl;
  }
}

bool Board::is_self_capture(Player player, Point point) {
  std::vector<std::shared_ptr<GoString>> friendly_strings;
  for (auto const& neighbor : neighbor_table_ptr->find(point)->second) {
    auto neighbor_string_it = grid.find(neighbor);
    if (neighbor_string_it == grid.end())
      return false;
    else if (neighbor_string_it->second->color == player)
      friendly_strings.push_back(neighbor_string_it->second);
    else if (neighbor_string_it->second->num_liberties() == 1)
      // This move is a real capture, not a self capture.
      return false;
  }
  return std::all_of(friendly_strings.begin(), friendly_strings.end(),
                     [](auto n){return n->num_liberties() == 1;});
}

bool Board::will_capture(Player player, Point point) {
  for (auto const& neighbor : neighbor_table_ptr->find(point)->second) {
    auto neighbor_string_it = grid.find(neighbor);
    if (neighbor_string_it == grid.end())
      continue;
    else if (neighbor_string_it->second->color == player)
      continue;
    else if (neighbor_string_it->second->num_liberties() == 1)
      // This move would capture.
      return true;
  }
  return false;
}


GameStatePtr GameState::apply_move(Move m) const {
  // std::cout << "In apply move " << m.is_play << "\n";
  BoardPtr next_board;
  if (m.is_play) {
    next_board = board->deepcopy();
    next_board->place_stone(next_player, m.point.value());
  } else {
    next_board.reset(new Board(*board)); // Shallow copy
  }
  return std::make_shared<GameState>(next_board, other_player(next_player), shared_from_this(), m, komi);
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

std::optional<Player> GameState::winner() const {
  if (! is_over())
    return std::nullopt;
  if (last_move && last_move.value().is_resign)
    return next_player;
  auto game_result = GameResult(board, komi);
  return game_result.winner();
}

bool GameState::is_move_self_capture(Player player, Move m) const {
  if (! m.is_play)
    return false;
  return board->is_self_capture(player, m.point.value());
}

bool GameState::does_move_violate_ko(Player player, Move m) const {
  if (! m.is_play)
    return false;
  if (! board->will_capture(player, m.point.value()))
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


std::vector<Move> GameState::legal_moves() const {
  std::vector<Move> moves;
  for (auto r=1; r <= board->num_rows; r++) {
    for (auto c=1; c <= board->num_cols; c++) {
      auto move = Move::play(Point(r, c));
      if (is_valid_move(move))
        moves.push_back(move);
    }
  }
  // These two moves are always legal:
  moves.push_back(Move::pass());
  moves.push_back(Move::resign());
  return moves;
}
