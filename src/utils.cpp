
#include <iostream>
#include <string>
#include <exception>
#include "utils.h"


static const std::string COLS = "ABCDEFGHJKLMNOPQRST";


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
