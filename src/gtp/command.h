#ifndef GTP_COMMAND_H
#define GTP_COMMAND_H

#include <string>
#include <sstream>
#include <optional>
#include <deque>
#include <iterator>
#include <stdexcept>
#include <cassert>


namespace {

  template <typename Out>
    void split_string(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
      *result++ = item;
    }
  }

  std::deque<std::string> split_string(const std::string &s, char delim = ' ') {
    std::deque<std::string> elems;
    split_string(s, delim, std::back_inserter(elems));
    return elems;
  }
}


namespace gtp {
  struct Command {
  public:
    std::optional<int> sequence;
    std::string name;
    std::deque<std::string> args;
  };

  Command parse_command(const std::string& command_string) {
    auto words = split_string(command_string);
    assert(words.size() > 0);

    int sequence;
    std::optional<int> sequence_opt;
    try {
      sequence = std::stoi(words[0]);
      sequence_opt = sequence;
      words.pop_front();
    }
    catch (const std::invalid_argument& e) {}

    auto name = words.front();
    words.pop_front();
      
    return Command({sequence_opt, name, words});
  }
}


#endif // GTP_COMMAND_H
