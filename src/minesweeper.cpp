#include "minesweeper.hpp"

void Board::initialize_board() {
    grid.resize(height, std::vector<Cell>(width));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x] = Cell();
        }
    }
}

void Board::calculate_adjacent_mines() {
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

void Board::place_mines(int safeX, int safeY) {
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

Board::Board(int w, int h, int mines) : width(w), height(h), mine_count(mines) {
    initialize_board();
}

Cell Board::get_cell(int x, int y) {
    return grid[y][x];
}

void Board::reveal_cell(int x, int y) {
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

void Board::toggle_flag(int x, int y) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;

    Cell& cell = grid[y][x];
    if (cell.is_revealed) return;

    cell.is_flagged = !cell.is_flagged;
    flagged_count += cell.is_flagged ? 1 : -1;
}

bool Board::is_game_over() const {
    return game_over;
}

bool Board::is_win() const {
    return revealed_count + flagged_count == width * height && mine_count == flagged_count;
}

void Board::reset() {
    revealed_count = 0;
    flagged_count = 0;
    first_click = true;
    game_over = false;
    initialize_board();
}