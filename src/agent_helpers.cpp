#include "agent_helpers.h"

bool is_point_an_eye(const Board& board, Point point, Player color) {
  // Must be an empty point:
  if (board.get(point))
    return false;

  // All adjacent points must contain friendly stones:
  for (const auto& neighbor : point.neighbors()) {
    if (board.is_on_grid(neighbor)) {
      auto neighbor_color = board.get(neighbor);
      if ((! neighbor_color) || neighbor_color.value() != color)
        return false;
    }
  }

  // Must control three out of four corners if the point is in the middle of the
  // board.  On the edge, must control all corners.
  int friendly_corners = 0;
  int off_board_corners = 0;
  const Point corners[] =  {
    Point(point.row - 1, point.col - 1),
    Point(point.row - 1, point.col + 1),
    Point(point.row + 1, point.col - 1),
    Point(point.row + 1, point.col + 1),
  };
  for (const auto& corner : corners) {
    if (board.is_on_grid(corner)) {
      auto corner_color = board.get(corner);
      if (corner_color && corner_color.value() == color)
        ++friendly_corners;
    }
    else
      ++off_board_corners;
  }
  if (off_board_corners > 0)
    return off_board_corners + friendly_corners == 4;
  return friendly_corners >= 3;
}
