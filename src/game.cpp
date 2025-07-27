#include "game.hpp"
#include "minesweeper.hpp"

using namespace blit;

// Screen and board configuration
constexpr int screen_size = 128;
constexpr int board_width = 16;
constexpr int board_height = 15;
constexpr int cell_size = screen_size / 16;
constexpr int mine_count = 20;

static Board board(board_width, board_height, mine_count);

// Cursor state
int cursor_x = 0;
int cursor_y = 0;

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

void draw_cursor() {
    screen.pen = Pen(255, 255, 0);
    int cx = (cursor_x * cell_size);
    int cy = (cursor_y * cell_size);
    int cs = cell_size;
    screen.line(Point(cx, cy), Point(cx + cs - 1, cy));
    screen.line(Point(cx, cy), Point(cx, cy + cs - 1));
    screen.line(Point(cx + cs - 1, cy), Point(cx + cs - 1, cy + cs - 1));
    screen.line(Point(cx, cy + cs - 1), Point(cx + cs - 1, cy + cs - 1));
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

void handle_cursor_movement() {
    if (buttons.pressed & Button::DPAD_LEFT) cursor_x = std::max(0, cursor_x - 1);
    if (buttons.pressed & Button::DPAD_RIGHT) cursor_x = std::min(board_width - 1, cursor_x + 1);
    if (buttons.pressed & Button::DPAD_UP) cursor_y = std::max(0, cursor_y - 1);
    if (buttons.pressed & Button::DPAD_DOWN) cursor_y = std::min(board_height - 1, cursor_y + 1);
}

void handle_cell_actions() {
    if (buttons.pressed & Button::A)
        board.reveal_cell(cursor_x, cursor_y);

    if (buttons.pressed & Button::B)
        board.toggle_flag(cursor_x, cursor_y);
}

void handle_game_reset() {
    if (buttons.pressed) {
        board.reset();
        cursor_x = 0;
        cursor_y = 0;
    }
}

void init() {
    set_screen_mode(ScreenMode::hires);
}

void update(uint32_t time) {
    if (board.is_game_over() || board.is_win()) {
        handle_game_reset();
        return;
    }
    handle_cursor_movement();
    handle_cell_actions();
}

void render(uint32_t time) {
    screen.pen = Pen(20, 20, 40);  // Dark blue background
    screen.clear();

    for (int y = 0; y < board_height; ++y) {
        for (int x = 0; x < board_width; ++x) {
            draw_cell(x, y, board.get_cell(x, y));
        }
    }

    draw_cursor();
    draw_status_text();
}
