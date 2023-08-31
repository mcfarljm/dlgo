
#include <iostream>
#include <string>
#include <exception>
#include "utils.h"


static const std::string COLS = "ABCDEFGHJKLMNOPQRST";


std::string stone_to_char(std::optional<Player> stone) {
  if (! stone)
    return " . ";
  else if (stone.value() == Player::white)
    return " o ";
  else
    return " x ";
}
    

void print_board(const Board& b) {
  for (auto row = b.num_rows; row > 0; row--) {
    auto pad = row <= 9 ? " " : "";
    std::string line;
    for (auto col = 1; col <= b.num_cols; col++) {
      auto stone = b.get(Point(row, col));
      line += stone_to_char(stone);
    }
    std::cout << pad << row << " " << line << std::endl;
  }
  std::cout << "    ";
  for (auto c=0; c< b.num_cols; c++) {
    std::cout << COLS[c] << "  ";
  }
  std::cout << std::endl;
}

void print_move(Player player, Move move) {
  std::string move_str;
  if (move.is_pass)
    move_str = "passes";
  else if (move.is_resign)
    move_str = "resigns";
  else
    move_str = COLS[move.point.value().col - 1] + std::to_string(move.point.value().row);
  std::cout << int(player) << " " << move_str << std::endl;
}


Point point_from_coords(std::string coords) {
  auto col = COLS.find(std::toupper(coords[0]));
  std::cout << "col: " << col << " " << std::string::npos;
  if (col == std::string::npos)
    throw bad_move_input();
  col++;
  coords.erase(0, 1);
  auto row = std::stoi(coords);
  return Point(row, col);
}


std::string lowercase(const std::string& s) {
  auto data = s;
  std::transform(data.begin(), data.end(), data.begin(),
                 [](unsigned char c){ return std::tolower(c); });
  return data;
}
