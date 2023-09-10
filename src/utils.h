#ifndef UTILS_H
#define UTILS_H

#include <exception>
#include <algorithm>
#include <string>
#include <chrono>

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


class Timer
{
public:
  Timer() : beg_(clock_::now()) {}
  void reset() { beg_ = clock_::now(); }
  double elapsed() const {
    return std::chrono::duration_cast<second_>
      (clock_::now() - beg_).count(); }

private:
  typedef std::chrono::steady_clock clock_;
  typedef std::chrono::duration<double, std::ratio<1> > second_;
  std::chrono::time_point<clock_> beg_;
};


#endif // UTILS_H
