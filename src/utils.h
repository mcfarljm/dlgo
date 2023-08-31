#ifndef UTILS_H
#define UTILS_H

#include <exception>
#include <algorithm>
#include <string>
#include "goboard.h"


std::string lowercase(const std::string& s);

void print_board(const Board& b);
void print_move(Player, Move);
Point point_from_coords(std::string);


class bad_move_input : public std::exception {
  virtual const char* what() const throw() {
    return "bad move input";
  }
};


#endif // UTILS_H
