#include <catch2/catch_test_macros.hpp>

#include "gotypes.h"
#include "goboard_slow.h"

TEST_CASE( "Check colors", "[colors]" ) {

  Player black = Player::black;
  Player white = Player::white;

  REQUIRE( other_player(black) == Player::white );
  REQUIRE( other_player(white) == Player::black );
  
}

TEST_CASE( "Check points", "[points]" ) {
  Point x {1, 2};
  Point y {1, 3};
  Point z {1, 2};

  REQUIRE( !(x == y) );
  REQUIRE( x == z );
}

TEST_CASE( "Test capture liberties", "[liberties]" ) {
  // Example from Figure 3.2
  Board board(6, 7);
  board.place_stone(Player::white, Point(3, 2));
  board.place_stone(Player::white, Point(4, 2));
  board.place_stone(Player::white, Point(5, 3));
  board.place_stone(Player::white, Point(5, 4));
  board.place_stone(Player::white, Point(4, 5));
  board.place_stone(Player::white, Point(3, 4));
  REQUIRE( board.grid.size() == 6);

  board.place_stone(Player::black, Point(3, 3));
  board.place_stone(Player::black, Point(4, 3));
  board.place_stone(Player::black, Point(4, 4));
  board.place_stone(Player::black, Point(3, 5));
  board.place_stone(Player::black, Point(3, 6));
  board.place_stone(Player::black, Point(4, 6));
  REQUIRE( board.grid.size() == 12);

  board.print();

  REQUIRE( board.grid.find(Point(3,2))->second->color == Player::white );
  REQUIRE( board.grid.find(Point(3,2))->second->stones.size() == 2 );
  REQUIRE( board.grid.find(Point(3,2))->second->num_liberties() == 4 );
  REQUIRE( board.grid.find(Point(5,3))->second->num_liberties() == 4 );
  REQUIRE( board.grid.find(Point(3,3))->second->color == Player::black );
  REQUIRE( board.grid.find(Point(3,3))->second->num_liberties() == 1 );
  REQUIRE( board.grid.find(Point(3,4))->second->num_liberties() == 1 );
  REQUIRE( board.grid.find(Point(3,5))->second->num_liberties() == 5 );

  // Black places stone to capture:
  board.place_stone(Player::black, Point(2, 4));

  board.print();

  REQUIRE( board.grid.find(Point(3,3))->second->num_liberties() == 2 );
  REQUIRE( board.grid.find(Point(4,6))->second->num_liberties() == 6 );
  
}