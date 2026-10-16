#pragma once

#include <cstdint>
#include <initializer_list>
#include <utility>

// Minesweeper rules: mine placement, flood reveal, flags, chording, win and
// loss. Plain C++ with no 32blit dependency, so tests/ can build it on the host.
namespace ms {

constexpr int MAX_W = 16;
constexpr int MAX_H = 16;

enum class State : uint8_t { READY, PLAYING, WON, LOST };

struct Cell {
  bool mine = false;
  bool revealed = false;
  bool flagged = false;
  bool exploded = false;   // the mine that ended the game
  bool wrong_flag = false; // flagged but not a mine, marked on loss
  uint8_t adjacent = 0;
  // Steps from where the reveal that opened this cell started, for the
  // ripple animation.
  uint8_t wave = 0;
  // Which reveal opened this cell (see Board::reveal_serial()).
  uint16_t serial = 0;
};

struct RevealResult {
  int revealed = 0;
  bool hit_mine = false;
  int max_wave = 0;
};

class Board {
public:
  // Clears the board. Mines are placed on the first reveal.
  void reset(int width, int height, int mines);
  // Places mines at fixed positions instead of randomly (tests and demos).
  void set_mines(std::initializer_list<std::pair<int, int>> positions);

  // Reveals a cell. The first reveal places the mines from seed, keeping the
  // 3x3 block around the cell clear so it always opens an area.
  RevealResult reveal(int x, int y, uint32_t seed);
  // On a revealed number whose neighbouring flags match it, reveals every
  // other hidden neighbour.
  RevealResult chord(int x, int y);
  bool can_chord(int x, int y) const;
  // Returns true if the flag changed.
  bool toggle_flag(int x, int y);

  State state() const { return state_; }
  bool finished() const { return state_ == State::WON || state_ == State::LOST; }
  int width() const { return width_; }
  int height() const { return height_; }
  int mine_count() const { return mines_; }
  int flags() const { return flags_; }
  int mines_left() const { return mines_ - flags_; }
  int safe_cells_left() const { return width_ * height_ - mines_ - revealed_; }
  int revealed_count() const { return revealed_; }
  int exploded_x() const { return exploded_x_; }
  int exploded_y() const { return exploded_y_; }
  uint16_t reveal_serial() const { return serial_; }

  bool in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }
  const Cell &at(int x, int y) const { return cells_[y * MAX_W + x]; }

private:
  Cell &cell(int x, int y) { return cells_[y * MAX_W + x]; }
  void place_mines(int safe_x, int safe_y, uint32_t seed);
  void count_adjacent();
  int flags_around(int x, int y) const;
  // Opens cells from the given starts, flooding through zeros.
  RevealResult open(const std::pair<int, int> *starts, int count);
  void lose(int x, int y);
  void win();

  Cell cells_[MAX_W * MAX_H];
  int width_ = 0, height_ = 0, mines_ = 0;
  int flags_ = 0, revealed_ = 0;
  int exploded_x_ = -1, exploded_y_ = -1;
  bool mines_placed_ = false;
  uint16_t serial_ = 0;
  State state_ = State::READY;
};

}
