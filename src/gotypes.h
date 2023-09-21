#ifndef GOTYPES_H
#define GOTYPES_H

#include <array>
#include <iostream>

enum class Player { black, white };

std::ostream& operator<<(std::ostream& os, Player player);

Player other_player(Player player);

struct Point {
  int row, col;
  Point(int r, int c) : row{r}, col{c} {}
  std::array<Point, 4> neighbors() const;
  bool operator==(const Point& rhs) const {
    return row == rhs.row && col == rhs.col;
  }
  bool operator<(const Point&rhs) const {
    if (row == rhs.row)
      return col < rhs.col;
    else
      return row < rhs.row;
  }
  friend std::ostream& operator<<(std::ostream&, Point);
};

class PointHash {
public:
  std::size_t operator() (const Point& p) const {
    return std::hash<int>()(p.row) ^ std::hash<int>()(p.col);
  }
};
  



#endif // GOTYPES_H
