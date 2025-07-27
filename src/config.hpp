#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include "32blit.hpp"
#include "minesweeper.hpp"

constexpr int screen_width = 128;
constexpr int screen_height = 128;
constexpr int header_height = 8;
constexpr int board_width = 16;
constexpr int board_height = 15;
constexpr int cell_size = screen_width / board_width;
constexpr int mine_count = 40;

#endif // CONFIG_HPP