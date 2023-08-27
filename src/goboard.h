#ifndef GOBOARD_H
#define GOBOARD_H

#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>
#include <iostream>
#include <utility> // Pair
#include <algorithm> // std::find
#include "gotypes.h"
#include "hash.h"

using PointSet = std::unordered_set<Point,PointHash>;
class GoString;
using GridMap = std::unordered_map<Point, std::shared_ptr<GoString>, PointHash>;
class Board;
using BoardPtr = std::shared_ptr<Board>;

// Initialize random hash keys:
static const Hasher hasher {};

class Move {
 public:

  std::optional<Point> point;
  bool is_play, is_pass, is_resign;

  Move(std::optional<Point> point, bool is_pass = false, bool is_resign = false)
    : point{point}, is_play{point.has_value()}, is_pass{is_pass}, is_resign{is_resign} {
    assert((point.has_value()) || is_pass || is_resign);
  }
  static Move play(Point point) {
    return Move(point);
  }
  static Move pass() {
    return Move({}, true);
  }
  static Move resign() {
    return Move({}, false, true);
  }
};

class GoString {
 private:

 public:
  Player color;
  PointSet stones;
  PointSet liberties;

  GoString(Player color, PointSet stones, PointSet liberties)
   : color{color}, stones{stones}, liberties{liberties} {}
  void remove_liberty(const Point &point) {liberties.erase(point);}
  void add_liberty(const Point &point) {liberties.insert(point);}
  GoString* merged_with(GoString &go_string) const {
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
  int num_liberties() const { return liberties.size(); }
  bool operator==(GoString const& rhs) const {
    return (color == rhs.color) && (stones == rhs.stones) && (liberties == rhs.liberties);
  }
};


GridMap deepcopy_grid(const GridMap&);


class Board {
private:
  uint64_t hash;
public:
  int num_rows, num_cols;
  std::unordered_map<Point, std::shared_ptr<GoString>, PointHash> grid;

  Board(int num_rows, int num_cols, GridMap grid = {}, uint64_t hash = 0)
    : num_rows{num_rows}, num_cols{num_cols}, grid(grid), hash(hash) {
    assert(num_rows <= HASH_MAX_BOARD && num_cols <= HASH_MAX_BOARD);
    // std::cout << "in board, hash: " << hasher.point_keys[0][0][0] << "\n";
  }

  BoardPtr deepcopy() {
    return std::make_shared<Board>(num_rows, num_cols, deepcopy_grid(grid), hash);
  }

  // Board(const Board& b) :
  //   num_rows(b.num_rows), num_cols(b.num_cols),
  //   grid(deepcopy_grid(b.grid)) {}

  bool is_on_grid(const Point& point) const {
    return 1 <= point.row && point.row <= num_rows &&
      1 <= point.col && point.col <= num_cols;
  }

  void place_stone(Player player, const Point& point);

  std::optional<Player> get(Point point) const {
    auto it = grid.find(point);
    if (it == grid.end())
      return std::nullopt;
    else
      return std::optional<Player>{it->second->color};
  }

  std::optional<std::shared_ptr<GoString>> get_go_string(Point point) const {
    auto it = grid.find(point);
    if (it == grid.end())
      return std::nullopt;
    else
      return it->second;
  }

  uint64_t get_hash() const { return hash; }

 private:
  void remove_string(std::shared_ptr<GoString> string);

};

class GameState : public std::enable_shared_from_this<GameState> {
private:
  std::shared_ptr<GameState> previous_state;
  std::optional<Move> last_move;
  // Todo: review whether this should be a set?
  std::vector<std::pair<Player, uint64_t>> previous_hashes;

public:
  Player next_player;
  BoardPtr board;
  GameState(BoardPtr board, Player next_player, std::shared_ptr<GameState> previous_state, std::optional<Move> last_move)
    : board{std::move(board)}, next_player{next_player}, previous_state{previous_state}, last_move{last_move} {
    if (previous_state) {
      previous_hashes = previous_state->previous_hashes;
      previous_hashes.push_back({previous_state->next_player, previous_state->board->get_hash()});
    }
  }

  std::shared_ptr<GameState> apply_move(Move m);

  static std::shared_ptr<GameState> new_game(int board_size) {
    auto board = std::make_shared<Board>(board_size, board_size);
    return std::make_shared<GameState>(board, Player::black, std::shared_ptr<GameState>(), std::nullopt);
  }

  bool is_over() const {
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

  bool is_move_self_capture(Player player, Move m) const {
    if (! m.is_play)
      return false;
    auto next_board = board->deepcopy();
    next_board->place_stone(player, m.point.value());
    auto new_string = next_board->get_go_string(m.point.value());
    assert(new_string);
    return new_string.value()->num_liberties() == 0;
  }

  bool does_move_violate_ko(Player player, Move m) const {
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

  bool is_valid_move(Move m) const {
    if (is_over())
      return false;
    if (m.is_pass || m.is_resign)
      return true;
    return (! board->get(m.point.value())) &&
      (! is_move_self_capture(next_player, m)) &&
      (! does_move_violate_ko(next_player, m));
  }

};

#endif // GOBOARD_H
