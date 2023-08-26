#ifndef GOTYPES_H
#define GOTYPES_H

#include <vector>

enum class Player { black, white };

Player other_player(Player &player);

struct Point {
  int row, col;
  Point(int r, int c) : row{r}, col{c} {}
  std::vector<Point> neighbors() const;
  bool operator==(const Point& rhs) const {
    return row == rhs.row && col == rhs.col;
  }
};

class PointHash {
public:
  std::size_t operator() (const Point& p) const {
    return std::hash<int>()(p.row) ^ std::hash<int>()(p.col);
  }
};
  



#endif // GOTYPES_H
