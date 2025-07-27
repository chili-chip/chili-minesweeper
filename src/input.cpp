#include "input.hpp"

using namespace blit;

Cursor::Cursor(int startX, int startY) : x(startX), y(startY) {}

void Cursor::move_left() {
    x = std::max(0, x - 1);
}

void Cursor::move_right() {
    x = std::min(board_width - 1, x + 1);
}

void Cursor::move_up() {
    y = std::max(0, y - 1);
}

void Cursor::move_down() {
    y = std::min(board_height - 1, y + 1);
}

void Cursor::reset() {
    x = 0;
    y = 0;
}

void Cursor::draw() {
    screen.sprite(1, Point((x * cell_size), (y * cell_size) + header_height));
}

Input::Input(Minesweeper &b) : board(b) {}

void Input::handle_cursor_movement() {
    if (buttons.pressed & Button::DPAD_LEFT) cursor.move_left();
    if (buttons.pressed & Button::DPAD_RIGHT) cursor.move_right();
    if (buttons.pressed & Button::DPAD_UP) cursor.move_up();
    if (buttons.pressed & Button::DPAD_DOWN) cursor.move_down();
}

void Input::handle_cell_actions() {
    if (buttons.released & Button::A)
        board.reveal_cell(cursor.get_x(), cursor.get_y());

    if (buttons.released & Button::B)
        board.toggle_flag(cursor.get_x(), cursor.get_y());
}

void Input::handle_game_reset() {
    if ((buttons.released & Button::A) || (buttons.released & Button::B)) {
        board.reset();
        cursor.reset();
        GameTimer::reset();
        GameTimer::start();
    }
}