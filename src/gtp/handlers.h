#ifndef HANDLERS_H
#define HANDLERS_H

#include <iostream>
#include <memory>
#include <unordered_map>
#include <deque>

#include "command.h"
#include "response.h"


namespace gtp {

  using ArgList = std::deque<std::string>;

  class CommandHandlerBase {

    std::unordered_map<std::string, Response (GTPFrontend::*)(const ArgList&)> handlers = {
      {"clear_board", &GTPFrontend::handle_clear_board},
      {"known_command", &GTPFrontend::handle_known_command},
      {"quit", &GTPFrontend::handle_quit},
    };

    Response process(Command cmd) {
      Response (GTPFrontend::*handler)(const ArgList&);
      auto it = handlers.find(cmd.name);
      if (it != handlers.end())
        handler = it->second;
      else
        handler = &GTPFrontend::handle_unknown;
      return (this->*handler)(cmd.args);
    }

    Response handle_clear_board(const ArgList& args) {
      game_state = GameState::new_game(19); // Todo: other board sizes?
      return Response::success();
    }

    Response handle_known_command(const ArgList& args) {
      return Response::bool_response(handlers.find(args[0]) != handlers.end());
    }

    Response handle_quit(const ArgList& args) {
      stopped = true;
      return Response::success();
    }

    Response handle_unknown(const ArgList& args) {
      return Response::error("Urecognized command");
    }

    

  };

}

#endif // HANDLERS_H
