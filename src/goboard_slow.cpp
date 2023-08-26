#include "goboard_slow.h"



void Board::place_stone(Player player, const Point& point) {
  assert(is_on_grid(point));
  assert(grid.find(point) == grid.end());

  std::unordered_set<std::shared_ptr<GoString>> adjacent_same_color;
  std::unordered_set<std::shared_ptr<GoString>> adjacent_opposite_color;
  PointSet liberties;

  for (const auto &neighbor : point.neighbors()) {
    if (! is_on_grid(neighbor)) continue;
    auto neighbor_string_it = grid.find(neighbor);
    if (neighbor_string_it == grid.end())
      liberties.insert(neighbor);
    else if (neighbor_string_it->second->color == player) {
      if (adjacent_same_color.find(neighbor_string_it->second) == adjacent_same_color.end())
        adjacent_same_color.insert(neighbor_string_it->second);
    }
    else if (adjacent_opposite_color.find(neighbor_string_it->second) == adjacent_opposite_color.end())
      adjacent_opposite_color.insert(neighbor_string_it->second);
  }
  PointSet single_point({point});
  std::shared_ptr<GoString> new_string
    (new GoString(player, std::move(single_point), liberties));
  for (const auto &same_color_string : adjacent_same_color)
    new_string.reset(new_string->merged_with(*same_color_string));
  for (const auto &new_string_point : new_string->stones)
    grid[new_string_point] = new_string;
  for (const auto &other_color_string : adjacent_opposite_color)
    other_color_string->remove_liberty(point);
  for (const auto &other_color_string : adjacent_opposite_color)
    if (other_color_string->num_liberties() == 0)
      remove_string(other_color_string);
}



void Board::remove_string(std::shared_ptr<GoString> string) {
  for (const auto& point : string->stones) {
    for (const auto& neighbor : point.neighbors()) {
      auto neighbor_string_it = grid.find(neighbor);
      if (neighbor_string_it == grid.end())
        continue;
      else if (neighbor_string_it->second != string)
        neighbor_string_it->second->add_liberty(point);
    }
    grid.erase(point);
  }
}
