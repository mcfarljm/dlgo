#ifndef GTP_RESPONSE_H
#define GTP_RESPONSE_H

#include <string>
#include <sstream>
#include "command.h"

namespace gtp {

  struct Response {
    bool is_success;
    std::string body;

    static Response success(std::string body = "") {
      return Response({true, body});
    }

    static Response error(std::string body = "") {
      return Response({false, body});
    }

    static Response bool_response(bool b) {
      return b ? success("true") : success("false");
    }

    std::string serialize(Command cmd) {
      std::stringstream ss;
      if (is_success)
        ss << "=";
      else
        ss << "?";
      if (cmd.sequence)
        ss << cmd.sequence.value();
      ss << " " << body << "\n\n";
      
      return ss.str();
    }
    
  };

}

#endif // GTP_RESPONSE_H
