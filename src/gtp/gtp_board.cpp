#include <sstream>

#include "gtp_board.h"

namespace {
  static const std::string COLS = "ABCDEFGHJKLMNOPQRST";
}

std::string coords_to_gtp_position(Move m) {
  std::stringstream ss;
  auto point = m.point.value();
  ss << COLS[point.col - 1];
  ss << point.row;
  return ss.str();
}

Move gtp_position_to_move(const std::string& s) {
  auto col_str = s[0];
  auto row_str = s.substr(1);
  auto point = Point({std::stoi(row_str),
      static_cast<int>(COLS.find(std::toupper(col_str))) + 1});
  return Move::play(point);
}
