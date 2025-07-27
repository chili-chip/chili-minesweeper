#ifndef INPUT_HPP
#define INPUT_HPP

#include "32blit.hpp"
#include <cstdint>
#include "minesweeper.hpp"
#include "config.hpp"

using namespace blit;

class Cursor {
private:
    int x, y = 0;
public:
    Cursor() = default;
    Cursor(int startX, int startY);
    void move_left();
    void move_right();
    void move_up();
    void move_down();
    void reset();
    void draw();
    int get_x() const { return x; }
    int get_y() const { return y; }
    void set_position(int new_x, int new_y) {
        x = new_x;
        y = new_y;
    }
};

class Input {
private:
    Board &board;
public:
    Cursor cursor;
    Input(Board &b);
    void handle_cursor_movement();
    void handle_cell_actions();
    void handle_game_reset();
};

#endif // INPUT_HPP