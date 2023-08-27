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

TEST_CASE( "Check board", "[board]" ) {
  Board board(5, 5);
  REQUIRE( board.is_on_grid(Point(2, 2)) );
  REQUIRE( board.is_on_grid(Point(1, 1)) );
  REQUIRE( board.is_on_grid(Point(5, 5)) );
  REQUIRE( ! board.is_on_grid(Point(0, 1)) );
  REQUIRE( ! board.is_on_grid(Point(1, 0)) );
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

  // Test deep copy:
  auto new_board = board.deepcopy();
  REQUIRE( new_board->grid.find(Point(4,6))->second->num_liberties() == 6 );

  new_board->place_stone(Player::white, Point(2, 5));

  REQUIRE( new_board->grid.find(Point(4,6))->second->num_liberties() == 5 );
  REQUIRE( board.grid.find(Point(4,6))->second->num_liberties() == 6 );

  
}


TEST_CASE( "Test game state", "[gamestate]" ) {
  auto game = GameState::new_game(19);
  REQUIRE( ! game->is_over() );

  SECTION( "Test resign" ) {
    game = game->apply_move(Move::resign());
    REQUIRE( game->is_over() );
  }

  SECTION( "Test single pass" ) {
    game = game->apply_move(Move::pass());
    REQUIRE( ! game->is_over() );
  }

  SECTION( "Test double pass" ) {
    game = game->apply_move(Move::pass());
    game = game->apply_move(Move::pass());
    REQUIRE( game->is_over() );
  }
}


TEST_CASE( "Self capture", "[selfcapture]" ) {
  auto game = GameState::new_game(7);

  game = game->apply_move(Move::play(Point(1, 3)));
  game = game->apply_move(Move::play(Point(1, 2)));
  game = game->apply_move(Move::play(Point(2, 3)));
  game = game->apply_move(Move::play(Point(2, 2)));
  game = game->apply_move(Move::play(Point(3, 3)));
  game = game->apply_move(Move::play(Point(3, 2)));
  game = game->apply_move(Move::pass());
  game = game->apply_move(Move::play(Point(4, 3)));
  game = game->apply_move(Move::pass());
  game = game->apply_move(Move::play(Point(3, 4)));
  game = game->apply_move(Move::pass());
  game = game->apply_move(Move::play(Point(2, 4)));
  game = game->apply_move(Move::pass());
  game = game->apply_move(Move::play(Point(1, 5)));
  game->board->print();

  // Example for Figure 3.3:
  auto m = Move::play(Point(1, 4));
  REQUIRE( game->is_move_self_capture(Player::black, m) );
  REQUIRE( ! game->is_move_self_capture(Player::white, m) );
}
