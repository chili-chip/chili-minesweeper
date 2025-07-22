#include "game.hpp"
#include "minesweeper.hpp"

using namespace blit;

// Screen and board configuration
constexpr int screen_size = 128;
constexpr int board_width = 16;
constexpr int board_height = 16;
constexpr int cell_size = 8;
constexpr int mine_count = 40;

static Board board(board_width, board_height, mine_count);

// Cursor state
int cursor_x = 0;
int cursor_y = 0;

void init() {
    set_screen_mode(ScreenMode::lores);
}

void update(uint32_t time) {
    if (buttons.released & Button::DPAD_LEFT) cursor_x = std::max(0, cursor_x - 1);
    if (buttons.released & Button::DPAD_RIGHT) cursor_x = std::min(board_width - 1, cursor_x + 1);
    if (buttons.released & Button::DPAD_UP) cursor_y = std::max(0, cursor_y - 1);
    if (buttons.released & Button::DPAD_DOWN) cursor_y = std::min(board_height - 1, cursor_y + 1);

    if (buttons.released & Button::A)
        board.reveal_cell(cursor_x, cursor_y);

    if (buttons.released & Button::B)
        board.toggle_flag(cursor_x, cursor_y);

    
}

void render(uint32_t time) {
    screen.clear();
    
    for (int y = 0; y < board_height; ++y) {
        for (int x = 0; x < board_width; ++x) {
            const Cell &cell = board.get_cell(x, y);
            Point pos(x * cell_size, y * cell_size);

            // Cell background
            if (cell.is_revealed) {
                screen.pen = cell.is_mine ? Pen(180, 0, 0) : Pen(200, 200, 200);
            } else {
                screen.pen = Pen(80, 80, 80);
            }
            screen.rectangle(Rect(pos, Size(cell_size - 1, cell_size - 1)));

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
    }

    // Draw cursor
    screen.pen = Pen(255, 255, 255);
    screen.rectangle(Rect(cursor_x * cell_size, cursor_y * cell_size, cell_size - 1, cell_size - 1));

    // Game over / win text
    if (board.is_game_over()) {
        screen.pen = Pen(255, 0, 0);
        screen.text("GAME OVER", minimal_font, Point(5, 120));
    } else if (board.is_win()) {
        screen.pen = Pen(0, 255, 0);
        screen.text("YOU WIN", minimal_font, Point(5, 120));
    }
}
