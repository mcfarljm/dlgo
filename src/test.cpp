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
