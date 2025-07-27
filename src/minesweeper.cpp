#include "minesweeper.hpp"

using namespace blit;

void Cell::draw(int x, int y) {
    Point pos((x * cell_size)+1, (y * cell_size)+1);

    // Cell background
    if (is_revealed) {
        screen.pen = is_mine ? Pen(180, 0, 0) : Pen(200, 200, 200);
    } else {
        screen.pen = Pen(80, 80, 80);
    }
    screen.rectangle(Rect(pos, Size(cell_size - 2, cell_size - 2)));

    // Text
    if (is_revealed && !is_mine && adjacent_mines > 0) {
        screen.pen = Pen(0, 0, 0);
        screen.text(std::to_string(adjacent_mines), minimal_font, pos + Point(1, 0));
    }

    if (is_flagged) {
        screen.pen = Pen(255, 255, 0);
        screen.text("F", minimal_font, pos + Point(1, 0));
    }

}

void Minesweeper::initialize_board() {
    grid.resize(height, std::vector<Cell>(width));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x] = Cell();
        }
    }
}

void Minesweeper::calculate_adjacent_mines() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid[y][x].is_mine) {
                for (const auto& [dx, dy] : directions) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height && !grid[ny][nx].is_mine) {
                        grid[ny][nx].adjacent_mines += 1;
                    }
                }
            }
        }
    }
}

void Minesweeper::place_mines(int safeX, int safeY) {
    std::vector<std::pair<int, int>> positions;

    // Optional: exclude neighbors too (for safe first-click + flood start)
    auto is_safe = [&](int x, int y) {
        return !(std::abs(x - safeX) <= 1 && std::abs(y - safeY) <= 1);
    };

    // Step 1: Collect valid positions
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (is_safe(x, y))
                positions.emplace_back(x, y);
        }
    }

    // Step 2: Shuffle
    std::shuffle(positions.begin(), positions.end(), std::mt19937{std::random_device{}()});

    // Step 3: Place mines
    for (int i = 0; i < mine_count && i < (int)positions.size(); ++i) {
        auto [x, y] = positions[i];
        grid[y][x].is_mine = true;
    }
}

Minesweeper::Minesweeper(int w, int h, int mines) : width(w), height(h), mine_count(mines) {
    initialize_board();
}

Cell Minesweeper::get_cell(int x, int y) {
    return grid[y][x];
}

void Minesweeper::reveal_cell(int x, int y) {
    // Check bounds
    if (x < 0 || y < 0 || x >= width || y >= height) return;

    // Place mines on first click
    if (first_click) {
        place_mines(x, y);
        calculate_adjacent_mines();
        first_click = false;
    }

    Cell& cell = grid[y][x];
    if (cell.is_revealed || cell.is_flagged) return;

    if (cell.is_mine) {
        game_over = true;
        return;
    }

    // Reveal the cell
    cell.is_revealed = true;
    revealed_count++;

    // Reveal adjacent cells if no adjacent mines
    if (cell.adjacent_mines == 0) {
        for (const auto& [dx, dy] : directions) {
            reveal_cell(x + dx, y + dy);
        }
    }
}

void Minesweeper::toggle_flag(int x, int y) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;

    Cell& cell = grid[y][x];
    if (cell.is_revealed) return;

    cell.is_flagged = !cell.is_flagged;
    flagged_count += cell.is_flagged ? 1 : -1;
}

bool Minesweeper::is_game_over() const {
    return game_over;
}

bool Minesweeper::is_win() const {
    return revealed_count + flagged_count == width * height && mine_count == flagged_count;
}

void Minesweeper::reset() {
    revealed_count = 0;
    flagged_count = 0;
    first_click = true;
    game_over = false;
    initialize_board();
}

void Minesweeper::draw_game_over() {
    if (is_game_over()) {
        screen.pen = Pen(255, 0, 0);
        screen.text("GAME OVER", minimal_font, Point(5, 120));
    } else if (is_win()) {
        screen.pen = Pen(0, 255, 0);
        screen.text("YOU WIN", minimal_font, Point(5, 120));
    }
}

void Minesweeper::draw() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x].draw(x, y);
        }
    }
    draw_game_over();
}