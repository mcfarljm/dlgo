cmake_minimum_required(VERSION 3.10)

project(dlgo LANGUAGES CXX)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)


add_library(dlgo src/goboard.cpp src/gotypes.cpp)
add_executable(tests src/test.cpp)
target_link_libraries(tests PRIVATE dlgo Catch2::Catch2WithMain)

add_executable(prog src/prog.cpp)
target_link_libraries(prog PRIVATE dlgo)


list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(tests)
