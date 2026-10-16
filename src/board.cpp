#include "board.hpp"

#include <algorithm>
#include <cstdlib>

namespace ms {

static const int DX[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const int DY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

// xorshift32: same boards on every platform for a given seed.
static uint32_t next_random(uint32_t &s) {
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return s;
}

void Board::reset(int width, int height, int mines) {
  width_ = std::clamp(width, 1, MAX_W);
  height_ = std::clamp(height, 1, MAX_H);
  mines_ = std::clamp(mines, 0, width_ * height_ - 1);
  for(auto &c : cells_) c = Cell();
  flags_ = revealed_ = 0;
  exploded_x_ = exploded_y_ = -1;
  mines_placed_ = false;
  serial_ = 0;
  state_ = State::READY;
}

void Board::set_mines(std::initializer_list<std::pair<int, int>> positions) {
  for(auto &c : cells_) c.mine = false;
  mines_ = 0;
  for(auto [x, y] : positions) {
    if(!in_bounds(x, y) || cell(x, y).mine) continue;
    cell(x, y).mine = true;
    mines_++;
  }
  count_adjacent();
  mines_placed_ = true;
}

void Board::place_mines(int safe_x, int safe_y, uint32_t seed) {
  // Keep the 3x3 block around the first reveal clear when there is room,
  // otherwise just the cell itself.
  int area = 0;
  for(int i = 0; i < 8; i++) area += in_bounds(safe_x + DX[i], safe_y + DY[i]) ? 1 : 0;
  bool big_safe_zone = width_ * height_ - 1 - area >= mines_;

  int candidates[MAX_W * MAX_H];
  int n = 0;
  for(int y = 0; y < height_; y++) {
    for(int x = 0; x < width_; x++) {
      int ddx = std::abs(x - safe_x), ddy = std::abs(y - safe_y);
      bool safe = big_safe_zone ? (ddx <= 1 && ddy <= 1) : (ddx == 0 && ddy == 0);
      if(!safe) candidates[n++] = y * MAX_W + x;
    }
  }

  uint32_t s = seed ? seed : 0x9e3779b9u;
  for(int i = 0; i < mines_ && i < n; i++) {
    int j = i + int(next_random(s) % uint32_t(n - i));
    std::swap(candidates[i], candidates[j]);
    cells_[candidates[i]].mine = true;
  }

  count_adjacent();
  mines_placed_ = true;
}

void Board::count_adjacent() {
  for(int y = 0; y < height_; y++) {
    for(int x = 0; x < width_; x++) {
      int n = 0;
      for(int i = 0; i < 8; i++) {
        int nx = x + DX[i], ny = y + DY[i];
        if(in_bounds(nx, ny) && at(nx, ny).mine) n++;
      }
      cell(x, y).adjacent = uint8_t(n);
    }
  }
}

int Board::flags_around(int x, int y) const {
  int n = 0;
  for(int i = 0; i < 8; i++) {
    int nx = x + DX[i], ny = y + DY[i];
    if(in_bounds(nx, ny) && at(nx, ny).flagged) n++;
  }
  return n;
}

RevealResult Board::open(const std::pair<int, int> *starts, int count) {
  RevealResult result;
  serial_++;

  // Breadth first, so each cell's wave is its distance from the start.
  int queue[MAX_W * MAX_H];
  int head = 0, tail = 0;
  for(int i = 0; i < count; i++) {
    auto [x, y] = starts[i];
    Cell &c = cell(x, y);
    if(c.revealed || c.flagged) continue;
    c.revealed = true;
    c.wave = 0;
    c.serial = serial_;
    queue[tail++] = y * MAX_W + x;
  }

  int hit = -1;
  while(head < tail) {
    int idx = queue[head++];
    int x = idx % MAX_W, y = idx / MAX_W;
    Cell &c = cells_[idx];
    if(c.mine) {
      if(hit < 0) hit = idx;
      continue;
    }
    revealed_++;
    result.revealed++;
    result.max_wave = std::max(result.max_wave, int(c.wave));
    if(c.adjacent != 0) continue;

    for(int i = 0; i < 8; i++) {
      int nx = x + DX[i], ny = y + DY[i];
      if(!in_bounds(nx, ny)) continue;
      Cell &n = cell(nx, ny);
      if(n.revealed || n.flagged) continue;
      n.revealed = true;
      n.wave = uint8_t(std::min(255, c.wave + 1));
      n.serial = serial_;
      queue[tail++] = ny * MAX_W + nx;
    }
  }

  if(hit >= 0) {
    result.hit_mine = true;
    lose(hit % MAX_W, hit / MAX_W);
  } else if(safe_cells_left() == 0) {
    win();
  }
  return result;
}

RevealResult Board::reveal(int x, int y, uint32_t seed) {
  if(!in_bounds(x, y) || finished()) return {};
  if(at(x, y).revealed || at(x, y).flagged) return {};

  if(!mines_placed_) place_mines(x, y, seed);
  state_ = State::PLAYING;

  std::pair<int, int> start{x, y};
  return open(&start, 1);
}

bool Board::can_chord(int x, int y) const {
  if(!in_bounds(x, y) || finished()) return false;
  const Cell &c = at(x, y);
  if(!c.revealed || c.mine || c.adjacent == 0) return false;
  if(flags_around(x, y) != c.adjacent) return false;
  for(int i = 0; i < 8; i++) {
    int nx = x + DX[i], ny = y + DY[i];
    if(in_bounds(nx, ny) && !at(nx, ny).revealed && !at(nx, ny).flagged) return true;
  }
  return false;
}

RevealResult Board::chord(int x, int y) {
  if(!can_chord(x, y)) return {};
  std::pair<int, int> starts[8];
  int n = 0;
  for(int i = 0; i < 8; i++) {
    int nx = x + DX[i], ny = y + DY[i];
    if(in_bounds(nx, ny) && !at(nx, ny).revealed && !at(nx, ny).flagged) starts[n++] = {nx, ny};
  }
  return open(starts, n);
}

bool Board::toggle_flag(int x, int y) {
  if(!in_bounds(x, y) || finished()) return false;
  Cell &c = cell(x, y);
  if(c.revealed) return false;
  c.flagged = !c.flagged;
  flags_ += c.flagged ? 1 : -1;
  return true;
}

void Board::lose(int x, int y) {
  state_ = State::LOST;
  exploded_x_ = x;
  exploded_y_ = y;
  cell(x, y).exploded = true;
  for(int yy = 0; yy < height_; yy++) {
    for(int xx = 0; xx < width_; xx++) {
      Cell &c = cell(xx, yy);
      if(c.mine && !c.flagged) c.revealed = true;
      if(c.flagged && !c.mine) c.wrong_flag = true;
    }
  }
}

void Board::win() {
  state_ = State::WON;
  for(int y = 0; y < height_; y++) {
    for(int x = 0; x < width_; x++) {
      Cell &c = cell(x, y);
      if(c.mine && !c.flagged) {
        c.flagged = true;
        flags_++;
      }
    }
  }
}

}
