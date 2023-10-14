cmake_minimum_required(VERSION 3.10)

project(dlgo LANGUAGES CXX)

find_package(Torch REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts
  GIT_TAG        v3.1.1
)

FetchContent_MakeAvailable(cxxopts)


# To use:
# - only works with static linking
# - compile in new dir: cmake .. -DCMAKE_BUILD_TYPE=PROFILE
# - run executable as normal
# - run gprof -P <exename> > prof.out
# The -P is optional but produces only the call graph, which is much easier to use.
set( CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_DEBUG} -pg" )

add_library(dlgo SHARED
  src/utils.cpp
  src/goboard.cpp
  src/gotypes.cpp
  src/agent_helpers.cpp
  src/agent_naive.cpp
  src/myrand.cpp

  src/simulation.cpp
  src/eval.cpp
  src/scoring.cpp
  src/alphabeta.cpp
  src/mcts.cpp

  src/gtp/command.h
  src/gtp/response.h
  src/gtp/frontend.h
  src/gtp/gtp_board.cpp

  src/zero/experience.cpp
  src/zero/encoder.cpp
  src/zero/agent_zero.cpp
)

target_link_libraries(dlgo "${TORCH_LIBRARIES}")

add_executable(tests src/test.cpp)
target_link_libraries(tests PRIVATE dlgo Catch2::Catch2WithMain)

add_executable(bot_v_bot src/bot_v_bot.cpp)
target_link_libraries(bot_v_bot PRIVATE dlgo)

add_executable(human_v_bot src/human_v_bot.cpp)
target_link_libraries(human_v_bot PRIVATE dlgo)

add_executable(dlgobot src/gtp/gtp_main.cpp)
target_link_libraries(dlgobot PRIVATE dlgo cxxopts)

add_executable(zero_sim src/zero_sim.cpp)
target_link_libraries(zero_sim PRIVATE dlgo cxxopts)

add_executable(matchup src/matchup.cpp)
target_link_libraries(matchup PRIVATE dlgo cxxopts)


# Don't build all exes by default
set_target_properties(bot_v_bot human_v_bot PROPERTIES EXCLUDE_FROM_ALL 1)


list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(tests)
