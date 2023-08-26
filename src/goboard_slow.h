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

class Move {
 private:
  std::optional<Point> point;
  bool is_play, is_pass, is_resign;

 public:

  Move(std::optional<Point> point, bool is_pass = false, bool is_resign = false)
    : point{point}, is_play{point.has_value()}, is_pass{is_pass}, is_resign{is_resign} {
    assert((point.has_value()) || is_pass || is_resign);
  }
  static Move play(Point point) {
    return Move(std::optional<Point>{point});
  }
  static Move pass_turn() {
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


class Board {
 private:
 public:
  int num_rows, num_cols;
  std::unordered_map<Point, std::shared_ptr<GoString>, PointHash> grid;
  Board(int num_rows, int num_cols)
    : num_rows{num_rows}, num_cols{num_cols} {}

  bool is_on_grid(const Point& point) const {
    return (1 <= point.row <= num_rows) &&
      (1 <= point.col <= num_cols);
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

#endif // GOBOARD_SLOW_H
