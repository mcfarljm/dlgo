
#include <iostream>
#include <string>
#include "utils.h"


static const char COLS[] = "ABCDEFGHJKLMNOPQRST";


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


