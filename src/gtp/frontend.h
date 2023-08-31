#ifndef GTP_FRONTEND_H
#define GTP_FRONTEND_H

#include <iostream>
#include <memory>
#include <unordered_map>
#include <deque>

#include "../goboard.h"
#include "../utils.h"
#include "command.h"
#include "response.h"
#include "gtp_board.h"
#include "../agent_base.h"


namespace gtp {

  using ArgList = std::deque<std::string>;
  
  class GTPFrontend {
  private:
    bool stopped;
    int boardsize = 19;
    std::unique_ptr<Agent> agent;

    std::istream& input = std::cin;
    std::ostream& output = std::cout;
    std::shared_ptr<GameState> game_state = GameState::new_game(19);

    std::unordered_map<std::string, Response (GTPFrontend::*)(const ArgList&)> handlers = {
      {"clear_board", &GTPFrontend::handle_clear_board},
      {"boardsize", &GTPFrontend::handle_boardsize},
      {"known_command", &GTPFrontend::handle_known_command},
      {"showbard", &GTPFrontend::handle_showboard},
      {"play", &GTPFrontend::handle_play},
      {"protocol_versoin", &GTPFrontend::handle_protocol_version},
      {"genmove", &GTPFrontend::handle_genmove},
      {"komi", &GTPFrontend::ignore},
      {"time_settings", &GTPFrontend::ignore},
      {"time_left", &GTPFrontend::ignore},
      {"quit", &GTPFrontend::handle_quit},
    };

  public:

    void run() {
      std::string line;
      while (! stopped) {
        getline(input, line);
        auto command = parse_command(line);
        auto response = process(command);
        output << response.serialize(command);
        output << std::flush;
      }
    }

  private:

    Response process(Command cmd) {
      Response (GTPFrontend::*handler)(const ArgList&);
      auto it = handlers.find(cmd.name);
      if (it != handlers.end())
        handler = it->second;
      else
        handler = &GTPFrontend::handle_unknown;
      return (this->*handler)(cmd.args);
    }

    /* Command handlers: */

    Response handle_clear_board(const ArgList& args) {
      game_state = GameState::new_game(boardsize);
      return Response::success();
    }

    Response handle_boardsize(const ArgList& args) {
      int new_boardsize = std::stoi(args[0]);
      game_state = GameState::new_game(new_boardsize);
      return Response::success();
    }

    Response handle_known_command(const ArgList& args) {
      return Response::bool_response(handlers.find(args[0]) != handlers.end());
    }

    Response handle_quit(const ArgList& args) {
      stopped = true;
      return Response::success();
    }

    Response handle_showboard(const ArgList& args) {
      print_board(*game_state->board);
      return Response::success();
    }

    Response handle_play(const ArgList& args) {
      std::string color = args[0];
      std::string move = args[1];
      if (lowercase(move) == "pass")
        game_state = game_state->apply_move(Move::pass());
      else if (lowercase(move) == "resign")
        game_state = game_state->apply_move(Move::resign());
      else {
        game_state = game_state->apply_move(gtp_position_to_move(move));
      }
      return Response::success();
    }


    Response handle_genmove(const ArgList& args) {
      auto move = agent->select_move(*game_state);
      if (move.is_pass)
        return Response::success("pass");
      else if (move.is_resign)
        return Response::success("resign");
      else
        return Response::success(coords_to_gtp_position(move));
    }

    Response handle_protocol_version(const ArgList& args) {
      return Response::success("2");
    }

    Response ignore(const ArgList& args) {
      return Response::success();
    }

    Response handle_unknown(const ArgList& args) {
      return Response::error("Urecognized command");
    }
    
  };

}


#endif // GTP_FRONTEND_H
