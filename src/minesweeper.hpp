#ifndef MINESWEEPER_HPP
#define MINESWEEPER_HPP

#include <vector>
#include <utility>
#include <random>
#include <algorithm>
#include <cmath>
#include "32blit.hpp"
#include "config.hpp"

class Cell {
public:
    bool is_mine = false;
    bool is_revealed = false;
    bool is_flagged = false;
    int adjacent_mines = 0;

    Cell() = default;
    void draw(int x, int y);
};

class Minesweeper {
private:
    int width, height, mine_count;
    int revealed_count = 0;
    int flagged_count = 0;
    bool first_click = true;
    bool game_over = false;
    std::vector<std::vector<Cell>> grid;

    const std::vector<std::pair<int, int>> directions = {
        {-1,-1}, {-1,0}, {-1,1},
        {0,-1},          {0,1},
        {1,-1}, {1,0},   {1,1}
    };

    void initialize_board();
    void place_mines(int safe_x, int safe_y);
    void calculate_adjacent_mines();
    
public:
    Minesweeper(int w, int h, int mines);
    Cell get_cell(int x, int y);
    void reveal_cell(int x, int y);
    void toggle_flag(int x, int y);
    bool is_game_over() const;
    bool is_win() const;
    void reset();
    void draw_game_over();
    void draw();
};

#endif // MINESWEEPER_HPP
