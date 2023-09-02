#include "gotypes.h"

Player other_player(Player &player) {
  return Player(1 - (int) player);
}

std::vector<Point> Point::neighbors() const {
  return {
    Point{row-1, col},
    Point{row+1, col},
    Point{row, col-1},
    Point{row, col+1},
  };
}

std::ostream& operator<<(std::ostream& os, Point p) {
  os << "(" << p.row << "," << p.col << ")";
  return os;
}
