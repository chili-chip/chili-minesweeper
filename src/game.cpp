#include "game.hpp"

using namespace blit;

static Board board(board_width, board_height, mine_count);
static Input input(board);

void draw_cell(int x, int y, const Cell &cell) {
    Point pos((x * cell_size)+1, (y * cell_size)+1);

    // Cell background
    if (cell.is_revealed) {
        screen.pen = cell.is_mine ? Pen(180, 0, 0) : Pen(200, 200, 200);
    } else {
        screen.pen = Pen(80, 80, 80);
    }
    screen.rectangle(Rect(pos, Size(cell_size - 2, cell_size - 2)));

    // Text
    if (cell.is_revealed && !cell.is_mine && cell.adjacent_mines > 0) {
        screen.pen = Pen(0, 0, 0);
        screen.text(std::to_string(cell.adjacent_mines), minimal_font, pos + Point(1, 0));
    }

    if (cell.is_flagged) {
        screen.pen = Pen(255, 255, 0);
        screen.text("F", minimal_font, pos + Point(1, 0));
    }
}

void draw_status_text() {
    if (board.is_game_over()) {
        screen.pen = Pen(255, 0, 0);
        screen.text("GAME OVER", minimal_font, Point(5, 120));
    } else if (board.is_win()) {
        screen.pen = Pen(0, 255, 0);
        screen.text("YOU WIN", minimal_font, Point(5, 120));
    }
}

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

    for (int y = 0; y < board_height; ++y) {
        for (int x = 0; x < board_width; ++x) {
            draw_cell(x, y, board.get_cell(x, y));
        }
    }

    input.cursor.draw();
    draw_status_text();
}
