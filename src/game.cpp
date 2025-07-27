#include "game.hpp"

using namespace blit;

static Minesweeper board(board_width, board_height, mine_count);
static Input input(board);

void init() {
    set_screen_mode(ScreenMode::hires);
}

void update(uint32_t time) {
    if (board.is_game_over() || board.is_win()) {
        input.handle_game_reset();
        return;
    }
    input.handle_cursor_movement();
    input.handle_cell_actions();
}

void render(uint32_t time) {
    screen.pen = Pen(20, 20, 40);  // Dark blue background
    screen.clear();
    board.draw();
    input.cursor.draw();
}
