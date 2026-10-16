#pragma once

#include "32blit.hpp"
#include "board.hpp"

#include <string_view>

// All drawing: board, header, effects and text helpers.
namespace ms::gfx {

constexpr int HEADER_H = 8;

namespace colors {
  const blit::Pen bg(16, 20, 36);
  const blit::Pen text(236, 240, 250);
  const blit::Pen dim(130, 144, 180);
  const blit::Pen accent(255, 214, 70);
  const blit::Pen gold(255, 200, 60);
  const blit::Pen red(240, 70, 60);
  const blit::Pen green(110, 220, 120);
  const blit::Pen select(70, 100, 170, 160);
}

// Where a board sits on screen.
struct Layout {
  int cell = 8;
  blit::Point origin;
};

Layout layout_for(const Board &board, int cell);
blit::Point cell_px(const Layout &l, int x, int y);
blit::Point cell_centre(const Layout &l, int x, int y);

// How to draw the board this frame.
struct BoardView {
  uint32_t now = 0;
  // Ripple: cells opened by reveal number `anim_serial` appear one wave
  // every WAVE_MS after anim_start.
  uint16_t anim_serial = 0;
  uint32_t anim_start = 0;
  // Loss: mines appear ring by ring around the exploded one.
  bool losing = false;
  uint32_t lose_start = 0;
  // Draw every cell hidden (pause screen).
  bool hide_all = false;
  bool show_cursor = true;
  bool cursor_pressed = false;
  int cursor_x = 0, cursor_y = 0;
};

constexpr uint32_t WAVE_MS = 22;
constexpr uint32_t RING_MS = 70;
constexpr uint32_t RING_DELAY_MS = 250;

// Chebyshev distance of a cell from the exploded mine.
int ring_of(const Board &board, int x, int y);

void draw_board(const Board &board, const Layout &l, const BoardView &v);

enum class Face : uint8_t { SMILE, OH, DEAD, COOL };
void draw_header(int mines_left, uint32_t time_ms, Face face, uint32_t now);

// Small icons, also used by menus.
void draw_mine(blit::Point centre, int size);
void draw_flag(blit::Point topleft, int size);

// Offset applied to the board, for screen shake.
void set_shake(blit::Point offset);

// Particle bursts and floating popups.
void burst(blit::Point px, blit::Pen color, int count, float speed = 1.0f);
void confetti(int count);
void popup(blit::Point px, const char *text, blit::Pen color);
void update_effects(uint32_t dt);
void draw_effects();
void clear_effects();

void draw_logo(int y, uint32_t time);

void dim_screen(uint8_t alpha);
void panel(blit::Rect r);

void text(std::string_view s, blit::Point p, blit::Pen color,
          blit::TextAlign align = blit::TextAlign::top_left, bool shadow = true);
// Text scaled up by an integer factor (pixel-doubled).
void big_text(std::string_view s, blit::Point center, int scale, blit::Pen color);

// "12.3" style seconds with one decimal, capped at 999.9.
void format_time(char *buf, int size, uint32_t ms);

}
