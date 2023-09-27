#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "gotypes.h"
#include "goboard.h"
#include "agent_helpers.h"
// #include "utils.h"
#include "gtp/command.h"
#include "gtp/response.h"
#include "scoring.h"
#include "eval.h"
#include "alphabeta.h"
#include "mcts.h"
#include "zero/encoder.h"
#include "zero/agent_zero.h"
#include "zero/dihedral.h"

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

  std::cout << board;

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

  std::cout << board;

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
  std::cout << *game->board;

  // Example for Figure 3.3:
  auto m = Move::play(Point(1, 4));
  REQUIRE( game->is_move_self_capture(Player::black, m) );
  REQUIRE( ! game->is_move_self_capture(Player::white, m) );
}


TEST_CASE( "Test ko", "[ko]" ) {
  auto game = GameState::new_game(7);

  game = game->apply_move(Move::play(Point(4, 3)));
  game = game->apply_move(Move::play(Point(4, 4)));
  game = game->apply_move(Move::play(Point(3, 4)));
  game = game->apply_move(Move::play(Point(5, 5)));
  game = game->apply_move(Move::play(Point(5, 4)));
  game = game->apply_move(Move::play(Point(3, 5)));
  game = game->apply_move(Move::pass());
  game = game->apply_move(Move::play(Point(4, 6)));

  std::cout << "before legal black cap " << game->board->get_hash() << "\n";
  std::cout << *game->board;

  // Black captures (legal):
  auto m = Move::play(Point(4, 5));
  REQUIRE( ! game->does_move_violate_ko(Player::black, m) );
  game = game->apply_move(m);

  std::cout << "\nbefore illegal white cap " << game->board->get_hash() << "\n";
  std::cout << *game->board;
  game->does_move_violate_ko(Player::white, Move::play(Point(4,4)));

  // White can't capture back
  REQUIRE( game->does_move_violate_ko(Player::white, Move::play(Point(4, 4))) );

  // See what happens if white captures:
  // game = game->apply_move(Move::play(Point(4, 4)));
  // std::cout << "\nafter illegal white cap " << game->board->get_hash() << "\n";
  // std::cout << board;
  // game->does_move_violate_ko(Player::black, Move::play(Point(1,1)));

  // Make sure other moves work:
  REQUIRE( ! game->does_move_violate_ko(Player::white, Move::play(Point(2, 4))) );

}

TEST_CASE( "Test eyes", "[eyes]" ) {
  auto game = GameState::new_game(5);

  game = game->apply_move(Move::play(Point(1, 1)));
  game = game->apply_move(Move::play(Point(1, 2)));
  game = game->apply_move(Move::play(Point(2, 1)));
  game = game->apply_move(Move::play(Point(2, 2)));
  game = game->apply_move(Move::play(Point(3, 2)));
  game = game->apply_move(Move::play(Point(2, 3)));
  game = game->apply_move(Move::play(Point(3, 3)));
  game = game->apply_move(Move::play(Point(2, 4)));
  game = game->apply_move(Move::play(Point(3, 4)));
  game = game->apply_move(Move::play(Point(2, 5)));
  game = game->apply_move(Move::play(Point(3, 5)));

  // std::cout << board;

  // Prior to filling in (1,4), verify that there aren't eyes
  REQUIRE( ! is_point_an_eye(*game->board, Point(1, 3), Player::white) );
  REQUIRE( ! is_point_an_eye(*game->board, Point(1, 4), Player::white) );
  REQUIRE( ! is_point_an_eye(*game->board, Point(1, 5), Player::white) );

  game = game->apply_move(Move::play(Point(1, 4)));
  std::cout << *game->board;

  // Now there are two eyes:
  REQUIRE( is_point_an_eye(*game->board, Point(1, 3), Player::white) );
  REQUIRE( ! is_point_an_eye(*game->board, Point(1, 4), Player::white) );
  REQUIRE( is_point_an_eye(*game->board, Point(1, 5), Player::white) );

  // Make sure they aren't eyes for black:
  REQUIRE( ! is_point_an_eye(*game->board, Point(1, 3), Player::black) );
  REQUIRE( ! is_point_an_eye(*game->board, Point(1, 4), Player::black) );
  REQUIRE( ! is_point_an_eye(*game->board, Point(1, 5), Player::black) );

}


TEST_CASE( "Test GTP command parsing", "[gtp]" ) {
  auto command = gtp::parse_command("999 play white D4");
  REQUIRE( command.sequence.value() == 999 );
  REQUIRE( command.name == "play" );
  REQUIRE( command.args[0] == "white" );
  REQUIRE( command.args[1] == "D4" );

  command = gtp::parse_command("play white D4");
  REQUIRE( ! command.sequence );
  REQUIRE( command.name == "play" );
  REQUIRE( command.args[0] == "white" );
  REQUIRE( command.args[1] == "D4" );

  command = gtp::parse_command("100 clear_board");
  auto response = gtp::Response::success();
  REQUIRE( response.serialize(command) == "=100 \n\n" );

  command = gtp::parse_command("clear_board");
  response = gtp::Response::success();
  REQUIRE( response.serialize(command) == "= \n\n" );

}


TEST_CASE( "Test scoring", "[scoring]" ) {
  // .w.ww
  // wwww.
  // bbbww
  // .bbbb
  // .b.b.
  auto board = std::make_shared<Board>(5, 5);
  board->place_stone(Player::black, Point(1, 2));
  board->place_stone(Player::black, Point(1, 4));
  board->place_stone(Player::black, Point(2, 2));
  board->place_stone(Player::black, Point(2, 3));
  board->place_stone(Player::black, Point(2, 4));
  board->place_stone(Player::black, Point(2, 5));
  board->place_stone(Player::black, Point(3, 1));
  board->place_stone(Player::black, Point(3, 2));
  board->place_stone(Player::black, Point(3, 3));
  board->place_stone(Player::white, Point(3, 4));
  board->place_stone(Player::white, Point(3, 5));
  board->place_stone(Player::white, Point(4, 1));
  board->place_stone(Player::white, Point(4, 2));
  board->place_stone(Player::white, Point(4, 3));
  board->place_stone(Player::white, Point(4, 4));
  board->place_stone(Player::white, Point(5, 2));
  board->place_stone(Player::white, Point(5, 4));
  board->place_stone(Player::white, Point(5, 5));

  auto territory = Territory::evaluate_territory(board);
  REQUIRE(9 == territory.num_black_stones);
  REQUIRE(4 == territory.num_black_territory);
  REQUIRE(9 == territory.num_white_stones);
  REQUIRE(3 == territory.num_white_territory);
  REQUIRE(0 == territory.num_dame);
}


TEST_CASE( "Benchmark alpha beta", "[!benchmark][alphabeta]" ) {

  const int MIN_SCORE = -999999;
  auto game = GameState::new_game(9);
  BENCHMARK("alpha beta depth 2") {
   return alpha_beta_result(*game, 2, MIN_SCORE, MIN_SCORE, &capture_diff);
  };

}


TEST_CASE( "Test alpha beta for profiling", "[.alphabetaprof]" ) {
  // A test case for profiling
  const int MIN_SCORE = -999999;
  auto game = GameState::new_game(9);
  auto result = alpha_beta_result(*game, 2, MIN_SCORE, MIN_SCORE, &capture_diff);
}


TEST_CASE( "Test MCTS", "[.mcts]" ) {
  // A test case for profiling
  auto agent = MCTSAgent(100, 1.4);
  auto game = GameState::new_game(9);
  auto move = agent.select_move(*game);
}


TEST_CASE( "Benchmark MCTS", "[!benchmark][mctsbench]" ) {
  auto agent = MCTSAgent(10, 1.4);
  auto game = GameState::new_game(9);
  BENCHMARK("MCTS") {
    return agent.select_move(*game);
  };
}


TEST_CASE( "Benchmark simulate game", "[!benchmark][simgame]" ) {
  auto game = GameState::new_game(9);
  BENCHMARK("Simulate game") {
    return MCTSAgent::simulate_random_game(game);
  };
}

TEST_CASE( "Benchmark valid move", "[!benchmark][validmove]" ) {
  auto game = GameState::new_game(9);
  BENCHMARK("is_valid_move") {
    return game->is_valid_move(Move::play(Point(5,5)));
  };
}


TEST_CASE( "Benchmark first move", "[!benchmark][firstmove]" ) {
  auto game = GameState::new_game(19);
  BENCHMARK("first move") {
    return game->apply_move(Move::play(Point(5, 5)));
  };
}


TEST_CASE( "Frozenset", "[frozenset]") {
  auto s0 = FrozenSet({1, 2, 3});
  auto s1 = FrozenSet({1, 5});
  auto s2 = s0 + s1;
  auto s3 = s0 - s1;
  REQUIRE( 4 == s2.size() );
  REQUIRE( 2 == s3.size() );
}


TEST_CASE( "Simple encoder", "[encoder]") {
  auto encoder = SimpleEncoder(9);
  REQUIRE( encoder.decode_move_index(9*9).is_pass );
  REQUIRE( encoder.decode_move_index(0).point.value() == Point(1, 1) );
  REQUIRE( encoder.decode_move_index(9).point.value() == Point(2, 1) );

  auto game = GameState::new_game(9);
  auto tensor = encoder.encode(*game);
  auto tensor_a = tensor.accessor<float,3>();
  for (auto filter=0; filter<11; ++filter) {
    for (auto i=0; i<9; ++i) {
      for (auto j=0; j<9; j++) {
        auto val = (filter == 9) ? 1.0 : 0.0;
        REQUIRE( tensor_a[filter][i][j] == val );
      }
    }
  }
}

TEST_CASE( "Benchmark zero move", "[!benchmark][zeromove]" ) {
  constexpr auto board_size = 9;
  // Note computational cost may not scale linearly with num rounds, so this
  // should be loosely representative of actual use.
  constexpr auto num_rounds = 500;

  c10::InferenceMode guard;
  torch::jit::script::Module model;
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    model = torch::jit::load("../nn/nine/conv_4x64.ts");
  }
  catch (const c10::Error& e) {
    std::cerr << "error loading the model\n";
    throw;
  }

  auto encoder = std::make_shared<SimpleEncoder>(board_size);
  auto agent = ZeroAgent(model, encoder, num_rounds);

  auto game = GameState::new_game(9);
  BENCHMARK("Zero Move") {
    return agent.select_move(*game);
  };
}


TEST_CASE( "Dihedral transformations", "[dihedral]" ) {
  torch::Tensor x = torch::reshape(torch::arange(0, 8), {2, 2, 2});
  std::cout << x << std::endl;
  auto null_transform = Dihedral(0, false);

  REQUIRE( torch::equal(null_transform.forward(x), x) );
  REQUIRE( torch::equal(null_transform.inverse(x), x) );

  auto trans = Dihedral(1, true);
  REQUIRE( torch::equal(trans.inverse(trans.forward(x)), x) );
  std::cout << trans.forward(x) << std::endl;

  auto rand = Dihedral();
  REQUIRE( torch::equal(rand.inverse(rand.forward(x)), x) );

}
