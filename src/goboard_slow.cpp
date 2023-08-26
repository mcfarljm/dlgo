#include "goboard_slow.h"
#include <iostream>
#include <string>


void Board::place_stone(Player player, const Point& point) {
  assert(is_on_grid(point));
  assert(grid.find(point) == grid.end());

  std::unordered_set<std::shared_ptr<GoString>> adjacent_same_color;
  std::unordered_set<std::shared_ptr<GoString>> adjacent_opposite_color;
  PointSet liberties;

  // Check neighbors
  // std::cout << "Looping over neighbors\n";
  for (const auto &neighbor : point.neighbors()) {
    if (! is_on_grid(neighbor)) continue;
    auto neighbor_string_it = grid.find(neighbor);
    if (neighbor_string_it == grid.end()) {
      // std::cout << "Adding liberty\n";
      liberties.insert(neighbor);
    }
    else if (neighbor_string_it->second->color == player) {
      // std::cout << "Found adjacent same color\n";
      adjacent_same_color.insert(neighbor_string_it->second);
    }
    else {
      // std::cout << "Found adjacent opposite color\n";
      adjacent_opposite_color.insert(neighbor_string_it->second);
    }
  }

  PointSet single_point({point});
  std::shared_ptr<GoString> new_string
    (new GoString(player, std::move(single_point), liberties));
  // Merge new string with adjacent ones:
  // std::cout << "Merging new string with " << adjacent_same_color.size() << " adjacent\n";
  for (const auto &same_color_string : adjacent_same_color) {
    // std::cout << "merging new string with same color with " << same_color_string->stones.size() << " stones\n";
    new_string.reset(new_string->merged_with(*same_color_string));
  }
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


std::string stone_to_char(std::optional<Player> stone) {
  if (! stone)
    return " . ";
  else if (stone.value() == Player::white)
    return " o ";
  else
    return " x ";
}
    


void Board::print() const {
  for (auto row = num_rows; row > 0; row--) {
    std::string line;
    for (auto col = 1; col <= num_cols; col++) {
      auto stone = get(Point(row, col));
      line += stone_to_char(stone);
    }
    std::cout << row << " " << line << std::endl;
  }
}
