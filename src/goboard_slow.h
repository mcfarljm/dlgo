#ifndef GOBOARD_SLOW_H
#define GOBOARD_SLOW_H

#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>
#include <iostream>
#include "gotypes.h"

using PointSet = std::unordered_set<Point,PointHash>;
class GoString;
using GridMap = std::unordered_map<Point, std::shared_ptr<GoString>, PointHash>;

class Move {
 public:

  std::optional<Point> point;
  bool is_play, is_pass, is_resign;

  Move(std::optional<Point> point, bool is_pass = false, bool is_resign = false)
    : point{point}, is_play{point.has_value()}, is_pass{is_pass}, is_resign{is_resign} {
    assert((point.has_value()) || is_pass || is_resign);
  }
  static Move play(Point point) {
    return Move(std::optional<Point>{point});
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
  PointSet liberties;

 public:
  Player color;
  PointSet stones;

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
 public:
  int num_rows, num_cols;
  std::unordered_map<Point, std::shared_ptr<GoString>, PointHash> grid;

  Board(int num_rows, int num_cols)
    : num_rows{num_rows}, num_cols{num_cols} {}

  Board(int num_rows, int num_cols, GridMap grid)
    : num_rows{num_rows}, num_cols{num_cols}, grid(grid) {}

  std::unique_ptr<Board> deepcopy() {
    return std::unique_ptr<Board>(new Board(num_rows, num_cols,
                                            deepcopy_grid(grid)));
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

  void print() const;

 private:
  void remove_string(std::shared_ptr<GoString> string);

};

class GameState : public std::enable_shared_from_this<GameState> {
private:
  std::shared_ptr<Board> board;
  Player next_player;
  std::shared_ptr<GameState> previous_state;
  std::optional<Move> last_move;

public:
  GameState(std::shared_ptr<Board> board, Player next_player, std::shared_ptr<GameState> previous_state, std::optional<Move> last_move)
    : board{std::move(board)}, next_player{next_player}, previous_state{previous_state}, last_move{last_move} {}

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
    
};

#endif // GOBOARD_SLOW_H
