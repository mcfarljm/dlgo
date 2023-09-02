#ifndef SCORING_H
#define SCORING_H

#include <cmath>
#include <unordered_map>
#include "gotypes.h"
#include "goboard.h"


enum class TerritoryStatus {
  black_stone, white_stone,
  black_territory, white_territory,
  dame,
};


class Territory {
public:
  int num_black_territory = 0;
  int num_white_territory = 0;
  int num_black_stones = 0;
  int num_white_stones = 0;
  int num_dame = 0;

  Territory(std::unordered_map<Point, TerritoryStatus, PointHash> territory_map);

  static Territory evaluate_territory(BoardPtr board);
  
private:
  PointSet dame_points;
};


struct GameResult {
  int black;
  int white;
  float komi;

  GameResult(int black, int white, float komi=7.5) :
    black(black), white(white), komi(komi) {}

  GameResult(GameStatePtr game_state, float komi=7.5) : komi(komi) {
    auto territory = Territory::evaluate_territory(game_state->board);
    black = territory.num_black_territory + territory.num_black_stones;
    white = territory.num_white_territory + territory.num_white_stones;
  }

  Player winner() {
    if (black > white + komi)
      return Player::black;
    else
      return Player::white;
  }

  float winning_margin() {
    float white_adjusted = white + komi;
    return abs(black - white_adjusted);
  }

};


#endif // SCORING_H
