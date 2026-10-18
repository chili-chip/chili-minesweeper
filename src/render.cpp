#include "render.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace blit;

namespace ms::gfx {

static Point shake(0, 0);

void set_shake(Point offset) {
  shake = offset;
}

Layout layout_for(const Board &board, int cell) {
  Layout l;
  l.cell = cell;
  int w = board.width() * cell, h = board.height() * cell;
  l.origin = Point((128 - w) / 2, HEADER_H + (128 - HEADER_H - h) / 2);
  return l;
}

Point cell_px(const Layout &l, int x, int y) {
  return l.origin + Point(x * l.cell, y * l.cell) + shake;
}

Point cell_centre(const Layout &l, int x, int y) {
  return cell_px(l, x, y) + Point(l.cell / 2, l.cell / 2);
}

int ring_of(const Board &board, int x, int y) {
  return std::max(std::abs(x - board.exploded_x()), std::abs(y - board.exploded_y()));
}

// ---------------------------------------------------------------------------
// Cells

static const Pen hidden_base(78, 100, 152);
static const Pen hidden_light(134, 160, 212);
static const Pen hidden_dark(40, 52, 90);
static const Pen open_base(196, 202, 218);
static const Pen open_line(158, 166, 188);

static const Pen number_colors[9] = {
  Pen(0, 0, 0),
  Pen(30, 80, 230),   // 1 blue
  Pen(20, 140, 40),   // 2 green
  Pen(220, 40, 40),   // 3 red
  Pen(40, 30, 150),   // 4 navy
  Pen(150, 30, 30),   // 5 maroon
  Pen(20, 140, 140),  // 6 teal
  Pen(20, 20, 20),    // 7 black
  Pen(110, 110, 120), // 8 grey
};

// 3x5 digits, one row per byte (bit 2 = left column).
static const uint8_t digits[10][5] = {
  {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 3, 1, 7}, {5, 5, 7, 1, 1},
  {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 2, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

static void draw_digit(int d, Point centre, int scale) {
  int w = 3 * scale, h = 5 * scale;
  Point tl = centre - Point(w / 2, h / 2);
  for(int row = 0; row < 5; row++) {
    for(int col = 0; col < 3; col++) {
      if(digits[d][row] & (4 >> col)) screen.rectangle(Rect(tl.x + col * scale, tl.y + row * scale, scale, scale));
    }
  }
}

static void draw_hidden(Point p, int s) {
  screen.pen = hidden_base;
  screen.rectangle(Rect(p.x, p.y, s, s));
  screen.pen = hidden_light;
  screen.rectangle(Rect(p.x, p.y, s - 1, 1));
  screen.rectangle(Rect(p.x, p.y, 1, s - 1));
  screen.pen = hidden_dark;
  screen.rectangle(Rect(p.x, p.y + s - 1, s, 1));
  screen.rectangle(Rect(p.x + s - 1, p.y, 1, s));
}

static void draw_open(Point p, int s, Pen base) {
  screen.pen = base;
  screen.rectangle(Rect(p.x, p.y, s, s));
  screen.pen = open_line;
  screen.rectangle(Rect(p.x, p.y + s - 1, s, 1));
  screen.rectangle(Rect(p.x + s - 1, p.y, 1, s));
}

void draw_mine(Point c, int s) {
  int r = std::max(2, s / 3);
  screen.pen = Pen(20, 20, 30);
  screen.rectangle(Rect(c.x - r - 1, c.y, 2 * r + 3, 1));
  screen.rectangle(Rect(c.x, c.y - r - 1, 1, 2 * r + 3));
  int d = r * 7 / 10 + 1;
  screen.pixel(c + Point(-d, -d));
  screen.pixel(c + Point(d, -d));
  screen.pixel(c + Point(-d, d));
  screen.pixel(c + Point(d, d));
  screen.circle(c, r);
  screen.pen = Pen(240, 240, 255);
  int h = std::max(1, r / 2);
  screen.pixel(c - Point(h, h));
  if(s >= 12) screen.pixel(c - Point(h - 1, h));
}

// 8x8 flag: R cloth, K pole.
static const char *const flag_8[8] = {
  "........",
  "..RRK...",
  ".RRRK...",
  "..RRK...",
  "....K...",
  "....K...",
  "..KKKK..",
  "........",
};

// 12x12 flag for the big cells.
static const char *const flag_12[12] = {
  "............",
  "....RRK.....",
  "..RRRRK.....",
  ".RRRRRK.....",
  "..RRRRK.....",
  "....RRK.....",
  "......K.....",
  "......K.....",
  "......K.....",
  "...KKKKKK...",
  "..KKKKKKKK..",
  "............",
};

static void draw_bitmap(const char *const *rows, int n, Point p) {
  for(int y = 0; y < n; y++) {
    for(int x = 0; rows[y][x]; x++) {
      char c = rows[y][x];
      if(c == '.') continue;
      screen.pen = c == 'R' ? Pen(235, 40, 40) : Pen(24, 24, 34);
      screen.pixel(Point(p.x + x, p.y + y));
    }
  }
}

void draw_flag(Point p, int s) {
  if(s >= 12) draw_bitmap(flag_12, 12, p + Point((s - 12) / 2, (s - 12) / 2));
  else draw_bitmap(flag_8, 8, p + Point((s - 8) / 2, (s - 8) / 2));
}

static void draw_cross(Point p, int s) {
  screen.pen = Pen(240, 30, 30);
  for(int i = 1; i < s - 1; i++) {
    screen.pixel(Point(p.x + i, p.y + i));
    screen.pixel(Point(p.x + s - 1 - i, p.y + i));
  }
}

void draw_board(const Board &board, const Layout &l, const BoardView &v) {
  const int s = l.cell;
  const int scale = s >= 13 ? 2 : 1;

  // Frame around boards that don't fill the screen.
  Point o = l.origin + shake;
  screen.pen = Pen(44, 56, 96);
  screen.rectangle(Rect(o.x - 1, o.y - 1, board.width() * s + 2, board.height() * s + 2));

  for(int y = 0; y < board.height(); y++) {
    for(int x = 0; x < board.width(); x++) {
      const Cell &c = board.at(x, y);
      Point p = cell_px(l, x, y);

      bool open = c.revealed && !v.hide_all;
      bool fresh = false;
      if(open && c.serial == v.anim_serial && !c.mine) {
        uint32_t t = v.now - v.anim_start;
        uint32_t due = c.wave * WAVE_MS;
        if(t < due) open = false;
        else fresh = t < due + 70;
      }
      if(open && c.mine && !c.exploded && v.losing) {
        uint32_t due = RING_DELAY_MS + ring_of(board, x, y) * RING_MS;
        if(v.now - v.lose_start < due) open = false;
      }

      bool pressed = v.cursor_pressed && v.show_cursor && x == v.cursor_x && y == v.cursor_y && !c.flagged;

      if(!open) {
        if(pressed && !c.revealed) draw_open(p, s, open_base);
        else draw_hidden(p, s);
        if(c.flagged && !v.hide_all) {
          draw_flag(p, s);
          if(c.wrong_flag) draw_cross(p, s);
        }
        continue;
      }

      Pen base = open_base;
      if(c.exploded) base = ((v.now / 120) & 1) ? Pen(250, 70, 50) : Pen(220, 40, 30);
      else if(fresh) base = Pen(232, 236, 248);
      draw_open(p, s, base);

      if(c.mine) {
        draw_mine(p + Point(s / 2, s / 2), s);
      } else if(c.adjacent > 0) {
        screen.pen = number_colors[c.adjacent];
        draw_digit(c.adjacent, p + Point(s / 2, s / 2), scale);
      }
    }
  }

  if(v.show_cursor && board.in_bounds(v.cursor_x, v.cursor_y)) {
    Point p = cell_px(l, v.cursor_x, v.cursor_y);
    float pulse = 0.5f + 0.5f * sinf(v.now * 0.008f);
    Pen c(255, uint8_t(190 + 60 * pulse), uint8_t(40 + 60 * pulse));
    screen.pen = Pen(0, 0, 0, 160);
    screen.rectangle(Rect(p.x - 2, p.y - 2, s + 4, 1));
    screen.rectangle(Rect(p.x - 2, p.y + s + 1, s + 4, 1));
    screen.rectangle(Rect(p.x - 2, p.y - 1, 1, s + 2));
    screen.rectangle(Rect(p.x + s + 1, p.y - 1, 1, s + 2));
    screen.pen = c;
    screen.rectangle(Rect(p.x - 1, p.y - 1, s + 2, 1));
    screen.rectangle(Rect(p.x - 1, p.y + s, s + 2, 1));
    screen.rectangle(Rect(p.x - 1, p.y, 1, s));
    screen.rectangle(Rect(p.x + s, p.y, 1, s));
  }
}

// ---------------------------------------------------------------------------
// Header

static void led_box(Point p, int w, const char *s) {
  screen.pen = Pen(8, 6, 10);
  screen.rectangle(Rect(p.x, p.y, w, 7));
  screen.pen = Pen(255, 70, 50);
  screen.text(s, minimal_font, Point(p.x + w - 1, p.y + 1), true, TextAlign::top_right);
}

void draw_header(int mines_left, uint32_t time_ms, Face face, uint32_t now) {
  screen.pen = Pen(28, 36, 64);
  screen.rectangle(Rect(0, 0, 128, HEADER_H));
  screen.pen = Pen(44, 56, 96);
  screen.rectangle(Rect(0, HEADER_H - 1, 128, 1));

  char buf[12];
  snprintf(buf, sizeof(buf), "%03d", std::max(-99, std::min(999, mines_left)));
  draw_flag(Point(0, 0), 8);
  led_box(Point(9, 0), 17, buf);

  uint32_t secs = std::min<uint32_t>(999, time_ms / 1000);
  snprintf(buf, sizeof(buf), "%03lu", (unsigned long)secs);
  led_box(Point(110, 0), 17, buf);

  int idx = 3;
  switch(face) {
    case Face::SMILE: idx = 3; break;
    case Face::OH: idx = 4; break;
    case Face::DEAD: idx = 5; break;
    case Face::COOL: idx = 6; break;
  }
  int bob = (face == Face::COOL && ((now / 200) & 1)) ? -1 : 0;
  screen.sprite(idx, Point(60, bob));
}

// ---------------------------------------------------------------------------
// Effects

struct Particle {
  float x, y, vx, vy;
  int16_t life, max_life;
  Pen color;
};

struct Popup {
  float x, y;
  int16_t life;
  char text[12];
  Pen color;
};

static Particle particles[96];
static Popup popups[4];

static float frand() {
  return (blit::random() % 1000) / 1000.0f;
}

void burst(Point px, Pen color, int count, float speed) {
  for(auto &pt : particles) {
    if(count <= 0) break;
    if(pt.life > 0) continue;
    float ang = frand() * 6.283f;
    float spd = (0.02f + frand() * 0.06f) * speed;
    pt.x = px.x;
    pt.y = px.y;
    pt.vx = cosf(ang) * spd;
    pt.vy = sinf(ang) * spd - 0.03f;
    pt.max_life = pt.life = int16_t(300 + blit::random() % 400);
    pt.color = (blit::random() & 1) ? color : Pen(255, 240, 200);
    count--;
  }
}

void confetti(int count) {
  static const Pen cols[] = {Pen(255, 90, 90), Pen(255, 214, 70), Pen(110, 220, 120), Pen(90, 170, 255), Pen(220, 120, 255)};
  for(auto &pt : particles) {
    if(count <= 0) break;
    if(pt.life > 0) continue;
    pt.x = frand() * 128;
    pt.y = -2 - frand() * 30;
    pt.vx = (frand() - 0.5f) * 0.03f;
    pt.vy = 0.02f + frand() * 0.04f;
    pt.max_life = pt.life = int16_t(1500 + blit::random() % 1200);
    pt.color = cols[blit::random() % 5];
    count--;
  }
}

void popup(Point px, const char *s, Pen color) {
  Popup *slot = &popups[0];
  for(auto &p : popups) {
    if(p.life <= 0) { slot = &p; break; }
    if(p.life < slot->life) slot = &p;
  }
  slot->x = std::max(16, std::min(112, int(px.x)));
  slot->y = std::max(HEADER_H + 6, int(px.y));
  slot->life = 800;
  strncpy(slot->text, s, sizeof(slot->text) - 1);
  slot->text[sizeof(slot->text) - 1] = 0;
  slot->color = color;
}

void update_effects(uint32_t dt) {
  for(auto &pt : particles) {
    if(pt.life <= 0) continue;
    pt.life -= dt;
    pt.x += pt.vx * dt;
    pt.y += pt.vy * dt;
    pt.vy += 0.00012f * dt;
  }
  for(auto &p : popups) {
    if(p.life <= 0) continue;
    p.life -= dt;
    p.y -= 0.015f * dt;
  }
}

void draw_effects() {
  for(auto &pt : particles) {
    if(pt.life <= 0) continue;
    Pen c = pt.color;
    c.a = uint8_t(255 * std::min<int>(pt.life, pt.max_life / 2) / (pt.max_life / 2));
    screen.pen = c;
    screen.pixel(Point(int(pt.x), int(pt.y)));
  }
  for(auto &p : popups) {
    if(p.life <= 0) continue;
    uint8_t a = uint8_t(std::min(255, p.life * 255 / 300));
    Pen c = p.color;
    c.a = a;
    Point at(int(p.x), int(p.y));
    screen.pen = Pen(0, 0, 0, a);
    screen.text(p.text, minimal_font, at + Point(1, 1), true, TextAlign::center_center);
    screen.pen = c;
    screen.text(p.text, minimal_font, at, true, TextAlign::center_center);
  }
}

void clear_effects() {
  for(auto &pt : particles) pt.life = 0;
  for(auto &p : popups) p.life = 0;
}

// ---------------------------------------------------------------------------
// Logo: "MINE" big with a ticking mine, "SWEEPER" under it.

void draw_logo(int y, uint32_t time) {
  for(int i = 0; i < 4; i++) {
    char ch[2] = {"MINE"[i], 0};
    int wave = int(roundf(sinf(time * 0.005f + i * 0.8f) * 1.5f));
    big_text(ch, Point(30 + i * 15, y + 9 + wave), 3, Pen(255, 110 + i * 25, 60));
  }
  // The mine with a fizzing fuse.
  Point mc(100, y + 10);
  screen.pen = Pen(150, 160, 200);
  screen.circle(mc, 7);
  screen.rectangle(Rect(mc.x - 9, mc.y - 1, 19, 3));
  screen.rectangle(Rect(mc.x - 1, mc.y - 9, 3, 19));
  screen.pen = Pen(30, 30, 44);
  screen.circle(mc, 6);
  screen.rectangle(Rect(mc.x - 8, mc.y, 17, 1));
  screen.rectangle(Rect(mc.x, mc.y - 8, 1, 17));
  screen.pen = Pen(90, 90, 110);
  screen.circle(mc - Point(2, 2), 1);
  screen.pen = Pen(150, 120, 80);
  screen.line(mc + Point(4, -5), mc + Point(7, -9));
  if((time / 90) % 3 != 0) {
    screen.pen = ((time / 60) & 1) ? Pen(255, 230, 80) : Pen(255, 120, 40);
    screen.pixel(mc + Point(8, -10));
    screen.pixel(mc + Point(7 + int(time / 70 % 3) - 1, -11));
  }
  big_text("SWEEPER", Point(64, y + 27), 2, colors::text);
}

// ---------------------------------------------------------------------------
// UI helpers

void dim_screen(uint8_t alpha) {
  screen.pen = Pen(0, 0, 0, alpha);
  screen.rectangle(Rect(0, 0, 128, 128));
}

void panel(Rect r) {
  screen.pen = Pen(14, 18, 34, 235);
  screen.rectangle(r);
  screen.pen = Pen(90, 120, 200);
  screen.rectangle(Rect(r.x + 1, r.y, r.w - 2, 1));
  screen.rectangle(Rect(r.x + 1, r.y + r.h - 1, r.w - 2, 1));
  screen.rectangle(Rect(r.x, r.y + 1, 1, r.h - 2));
  screen.rectangle(Rect(r.x + r.w - 1, r.y + 1, 1, r.h - 2));
}

void text(std::string_view s, Point p, Pen color, TextAlign align, bool shadow) {
  if(shadow) {
    screen.pen = Pen(0, 0, 0, 200);
    screen.text(s, minimal_font, p + Point(1, 1), true, align);
  }
  screen.pen = color;
  screen.text(s, minimal_font, p, true, align);
}

void big_text(std::string_view s, Point center, int scale_by, Pen color) {
  static uint8_t buf[128 * 8 * 4];
  Surface tmp(buf, PixelFormat::RGBA, Size(128, 8));

  Size sz = screen.measure_text(s, minimal_font, true);
  int w = std::min(int(sz.w), 128);
  Rect src(0, 0, w, 8);
  Rect dst(center.x - w * scale_by / 2, center.y - 4 * scale_by, w * scale_by, 8 * scale_by);

  for(int pass = 0; pass < 2; pass++) {
    memset(buf, 0, sizeof(buf));
    tmp.pen = pass == 0 ? Pen(0, 0, 0, 220) : color;
    tmp.text(s, minimal_font, Point(0, 0), true);
    Rect d = dst;
    if(pass == 0) {
      d.x += scale_by / 2 + 1;
      d.y += scale_by / 2 + 1;
    }
    screen.stretch_blit(&tmp, src, d);
  }
}

void format_time(char *buf, int size, uint32_t ms) {
  uint32_t tenths = std::min<uint32_t>(9999, ms / 100);
  snprintf(buf, size, "%lu.%lu", (unsigned long)(tenths / 10), (unsigned long)(tenths % 10));
}

}
