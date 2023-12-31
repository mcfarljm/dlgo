#ifndef GOBOARD_H
#define GOBOARD_H

#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <optional>
#include <memory>
#include <iostream>
#include <utility> // Pair
#include <algorithm> // std::find
#include "gotypes.h"
#include "hash.h"
#include "frozenset.h"

using FrozenPointSet = FrozenSet<Point,PointHash>;
class GoString;
using GridMap = std::unordered_map<Point, std::shared_ptr<GoString>, PointHash>;
// using GridMap = std::map<Point, std::shared_ptr<GoString>>;
class Board;
using BoardPtr = std::shared_ptr<Board>;
using ConstBoardPtr = std::shared_ptr<const Board>;
class GameState;
using GameStatePtr = std::shared_ptr<GameState>;
using ConstGameStatePtr = std::shared_ptr<const GameState>;

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
  bool operator==(const Move& rhs) const {
    if (is_play && rhs.is_play)
      return point.value() == rhs.point.value();
    else
      return is_play == rhs.is_play && is_pass == rhs.is_pass && is_resign == rhs.is_resign;
  }
};

std::ostream& operator<<(std::ostream& os, const Move& m);


class MoveHash {
public:
  std::size_t operator() (const Move& m) const {
    auto hash = std::hash<bool>()(m.is_pass) ^ std::hash<bool>()(m.is_resign);
    if (m.is_play)
      hash ^= PointHash()(m.point.value());
    return hash;
  }
};


class GoString {
 private:

 public:
  Player color;
  FrozenPointSet stones;
  FrozenPointSet liberties;

  GoString(Player color, FrozenPointSet stones, FrozenPointSet liberties)
   : color{color}, stones{stones}, liberties{liberties} {}
  std::shared_ptr<GoString> without_liberty(Point point) {
    auto new_liberties = liberties - FrozenPointSet({point});
    return std::make_shared<GoString>(color, stones, new_liberties);
  }
  std::shared_ptr<GoString> with_liberty(Point point) {
    auto new_liberties = liberties + FrozenPointSet({point});
    return std::make_shared<GoString>(color, stones, new_liberties);
  }

  std::shared_ptr<GoString> merged_with(GoString &go_string) const {
    assert(go_string.color == color);
    auto combined_stones = stones + go_string.stones;
    return std::make_shared<GoString>(color, combined_stones,
                                      (liberties + go_string.liberties) - combined_stones);
  }

  
  int num_liberties() const { return liberties.size(); }
  bool operator==(GoString const& rhs) const {
    return (color == rhs.color) && (stones == rhs.stones) && (liberties == rhs.liberties);
  }
};


class Board {
private:
  uint64_t hash;
  static std::map<std::pair<int,int>,
                            std::unordered_map<Point, std::vector<Point>, PointHash>> neighbor_tables;
  static void init_neighbor_table(std::pair<int,int>);
  std::unordered_map<Point, std::vector<Point>, PointHash>* neighbor_table_ptr;
public:
  int num_rows, num_cols;
  GridMap grid;

  Board(int num_rows, int num_cols, GridMap grid = {}, uint64_t hash = 0)
    : num_rows{num_rows}, num_cols{num_cols}, grid(grid), hash(hash) {
    assert(num_rows <= HASH_MAX_BOARD && num_cols <= HASH_MAX_BOARD);
    // std::cout << "in board, hash: " << hasher.point_keys[0][0][0] << "\n";
    auto dim = std::make_pair(num_rows, num_cols);
    if (neighbor_tables.find(dim) == neighbor_tables.end())
      init_neighbor_table(dim);
    neighbor_table_ptr = &(neighbor_tables.find(dim)->second);
  }

  friend std::ostream& operator<<(std::ostream&, const Board& b);

  // This is a misnomer holdover from the "slow" implementation that requires
  // deep copies.  Current implementation uses immutable sets inside the grid
  // map, so we don't actually need to deep copy the grid.
  BoardPtr deepcopy() {
    return std::make_shared<Board>(num_rows, num_cols, grid, hash);
  }

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

  bool is_self_capture(Player, Point);
  bool will_capture(Player, Point);

 private:
  void replace_string(std::shared_ptr<GoString> string);
  void remove_string(std::shared_ptr<GoString> string);

};

class GameState : public std::enable_shared_from_this<GameState> {
private:
  ConstGameStatePtr previous_state;
  std::optional<Move> last_move;
  // Todo: review whether this should be a set?
  std::vector<std::pair<Player, uint64_t>> previous_hashes;
  float komi = 7.5;

public:
  Player next_player;
  BoardPtr board;
  int num_moves = 0;

  GameState(BoardPtr board, Player next_player, ConstGameStatePtr previous_state, std::optional<Move> last_move, float komi)
    : board{std::move(board)}, next_player{next_player}, previous_state{previous_state}, last_move{last_move}, komi{komi} {
    if (previous_state) {
      previous_hashes = previous_state->previous_hashes;
      previous_hashes.push_back({previous_state->next_player, previous_state->board->get_hash()});
      num_moves = previous_state->num_moves + 1;
    }
  }

  GameStatePtr apply_move(Move m) const;

  static GameStatePtr new_game(int board_size, float komi=7.5) {
    auto board = std::make_shared<Board>(board_size, board_size);
    return std::make_shared<GameState>(board, Player::black, GameStatePtr(), std::nullopt, komi);
  }

  bool is_over() const;

  std::optional<Player> winner() const;

  bool is_move_self_capture(Player player, Move m) const;

  bool does_move_violate_ko(Player player, Move m) const;

  bool is_valid_move(Move m) const;

  std::vector<Move> legal_moves() const;

};

#endif // GOBOARD_H
