# Host-side unit tests for the board rules. They build without the 32blit SDK:
#   cmake -S tests -B build-tests && cmake --build build-tests && ctest --test-dir build-tests
cmake_minimum_required(VERSION 3.15)
project(minesweeper_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
add_executable(board_tests test_board.cpp ../src/board.cpp)
target_include_directories(board_tests PRIVATE ../src)
if(MSVC)
  target_compile_options(board_tests PRIVATE /W4)
else()
  target_compile_options(board_tests PRIVATE -Wall -Wextra)
endif()
add_test(NAME board_tests COMMAND board_tests)
