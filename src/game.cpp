#include "32blit.hpp"
#include "game.hpp"

#include "assets.hpp"
#include "audio.hpp"
#include "board.hpp"
#include "render.hpp"
#include "save.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

using namespace blit;
using namespace ms;

namespace {

enum class Scene : uint8_t {
  TITLE,
  SCORES,
  HELP,
  CREDITS,
  PLAYING,
  PAUSED,
  EXPLODING,
  CLEARED,
  NAME_ENTRY,
  RESULTS,
};

enum MenuItem : uint8_t { M_PLAY, M_DIFFICULTY, M_MUSIC, M_SOUND, M_SCORES, M_HELP, M_CREDITS, M_COUNT };
enum PauseItem : uint8_t { P_RESUME, P_RESTART, P_MENU, P_COUNT };

// Cursor auto-repeat while a direction is held.
const uint32_t REPEAT_DELAY_MS = 220;
const uint32_t REPEAT_MS = 60;

const uint32_t CLEARED_MS = 1400;

Scene scene = Scene::TITLE;
uint32_t scene_start = 0;
uint32_t now_ms = 0;
uint32_t last_update = 0;

int menu_index = 0;
int pause_index = 0;
int scores_page = 0;

Board board;
gfx::Layout layout;
int difficulty = MEDIUM;
int cursor_x = 0, cursor_y = 0;
uint32_t play_ms = 0;  // game clock, runs from the first reveal
uint16_t anim_serial = 0;
uint32_t anim_start = 0;
uint32_t lose_start = 0;
int last_ring = -1;
int max_ring = 0;

bool won = false;
int new_rank = -1;
uint32_t best_at_start = 0;

char entry_name[4] = "AAA";
int entry_cursor = 0;

uint32_t shake_until = 0;
uint32_t flash_until = 0;

// Title screen demo: a board that solves itself in the background.
Board demo;
uint32_t demo_timer = 0;

uint32_t repeat_timer[4];
const Button dir_buttons[4] = {Button::DPAD_LEFT, Button::DPAD_RIGHT, Button::DPAD_UP, Button::DPAD_DOWN};

void enter(Scene s) {
  scene = s;
  scene_start = now_ms;
}

uint32_t in_scene() {
  return now_ms - scene_start;
}

const DifficultyInfo &info() {
  return difficulties[difficulty];
}

uint32_t best_time(int d) {
  return save.scores[d][0].time_ms;
}

void apply_audio_settings() {
  audio::set_music_enabled(save.music);
  audio::set_sfx_enabled(save.sound);
}

void start_game() {
  difficulty = save.difficulty;
  const DifficultyInfo &d = info();
  board.reset(d.width, d.height, d.mines);
  layout = gfx::layout_for(board, d.cell);
  cursor_x = d.width / 2;
  cursor_y = d.height / 2;
  play_ms = 0;
  anim_serial = 0;
  won = false;
  new_rank = -1;
  best_at_start = best_time(difficulty);
  last_ring = -1;
  gfx::clear_effects();
  audio::play_song(audio::Song::GAME);
  enter(Scene::PLAYING);
}

void reset_demo() {
  demo.reset(16, 16, 40);
  demo.reveal(int(blit::random() % 16), int(blit::random() % 16), blit::random());
}

void go_to_title() {
  reset_demo();
  audio::play_song(audio::Song::TITLE);
  enter(Scene::TITLE);
}

// Up/down menu navigation with wrap-around.
bool menu_nav(int &index, int count) {
  if(buttons.pressed & Button::DPAD_UP) {
    index = (index + count - 1) % count;
    audio::play_sfx(audio::Sfx::MENU_MOVE);
    return true;
  }
  if(buttons.pressed & Button::DPAD_DOWN) {
    index = (index + 1) % count;
    audio::play_sfx(audio::Sfx::MENU_MOVE);
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Update

void update_demo(uint32_t dt) {
  demo_timer += dt;
  if(demo_timer < 260) return;
  demo_timer = 0;
  if(demo.finished()) {
    reset_demo();
    return;
  }
  // Reveal a random safe cell next to an opened one, or flag a mine there.
  int tries = 0;
  while(tries++ < 200) {
    int x = int(blit::random() % 16), y = int(blit::random() % 16);
    const Cell &c = demo.at(x, y);
    if(c.revealed || c.flagged) continue;
    bool next_to_open = false;
    for(int dy = -1; dy <= 1; dy++)
      for(int dx = -1; dx <= 1; dx++)
        if(demo.in_bounds(x + dx, y + dy) && demo.at(x + dx, y + dy).revealed) next_to_open = true;
    if(!next_to_open && tries < 150) continue;
    if(c.mine) demo.toggle_flag(x, y);
    else demo.reveal(x, y, 0);
    return;
  }
}

void update_title() {
  menu_nav(menu_index, M_COUNT);

  int change = 0;
  if(buttons.pressed & Button::DPAD_LEFT) change = -1;
  if(buttons.pressed & (Button::DPAD_RIGHT | Button::A)) change = 1;
  if(!change) return;

  bool a = buttons.pressed & Button::A;
  switch(menu_index) {
    case M_PLAY:
      if(a) {
        audio::play_sfx(audio::Sfx::MENU_SELECT);
        start_game();
      }
      return;
    case M_DIFFICULTY:
      save.difficulty = (save.difficulty + DIFFICULTY_COUNT + change) % DIFFICULTY_COUNT;
      break;
    case M_MUSIC:
      save.music = !save.music;
      apply_audio_settings();
      if(save.music) audio::play_song(audio::Song::TITLE);
      break;
    case M_SOUND:
      save.sound = !save.sound;
      apply_audio_settings();
      break;
    case M_SCORES:
    case M_HELP:
    case M_CREDITS:
      if(a) {
        audio::play_sfx(audio::Sfx::MENU_SELECT);
        scores_page = save.difficulty;
        enter(menu_index == M_SCORES ? Scene::SCORES : menu_index == M_HELP ? Scene::HELP : Scene::CREDITS);
      }
      return;
  }
  audio::play_sfx(audio::Sfx::MENU_SELECT);
  write_save_data();
}

void update_info_screen() {
  if(scene == Scene::SCORES) {
    int change = 0;
    if(buttons.pressed & Button::DPAD_LEFT) change = -1;
    if(buttons.pressed & Button::DPAD_RIGHT) change = 1;
    if(change) {
      scores_page = (scores_page + DIFFICULTY_COUNT + change) % DIFFICULTY_COUNT;
      audio::play_sfx(audio::Sfx::MENU_MOVE);
    }
  }
  if(buttons.pressed & (Button::A | Button::B | Button::MENU)) {
    audio::play_sfx(audio::Sfx::MENU_BACK);
    enter(Scene::TITLE);
  }
}

bool move_cursor(int i) {
  int w = board.width(), h = board.height();
  switch(i) {
    case 0: cursor_x = (cursor_x + w - 1) % w; break;
    case 1: cursor_x = (cursor_x + 1) % w; break;
    case 2: cursor_y = (cursor_y + h - 1) % h; break;
    case 3: cursor_y = (cursor_y + 1) % h; break;
  }
  return true;
}

void handle_cursor(uint32_t dt) {
  bool moved = false;
  for(int i = 0; i < 4; i++) {
    Button b = dir_buttons[i];
    if(buttons.pressed & b) {
      moved |= move_cursor(i);
      repeat_timer[i] = REPEAT_DELAY_MS;
    } else if(buttons.state & b) {
      if(repeat_timer[i] <= dt) {
        moved |= move_cursor(i);
        repeat_timer[i] = REPEAT_MS - std::min(REPEAT_MS - 1, dt - repeat_timer[i]);
      } else {
        repeat_timer[i] -= dt;
      }
    }
  }
  if(moved) audio::play_sfx(audio::Sfx::CURSOR);
}

void finish_game() {
  record_game(difficulty, won);
  if(won) {
    new_rank = time_rank(difficulty, std::max<uint32_t>(1, play_ms));
    if(new_rank >= 0) {
      memcpy(entry_name, save.last_name, sizeof(entry_name));
      entry_cursor = 0;
      write_save_data();
      enter(Scene::NAME_ENTRY);
      return;
    }
  }
  write_save_data();
  enter(Scene::RESULTS);
}

void on_reveal(const RevealResult &r) {
  if(r.revealed == 0 && !r.hit_mine) return;
  anim_serial = board.reveal_serial();
  anim_start = now_ms;

  if(r.hit_mine) {
    audio::play_sfx(audio::Sfx::EXPLODE);
    audio::play_song(audio::Song::LOSE);
    Point at = gfx::cell_centre(layout, board.exploded_x(), board.exploded_y());
    gfx::burst(at, Pen(255, 140, 40), 40, 1.6f);
    gfx::burst(at, Pen(255, 60, 40), 20, 0.8f);
    shake_until = now_ms + 600;
    flash_until = now_ms + 160;
    lose_start = now_ms;
    last_ring = 0;
    max_ring = 0;
    for(int y = 0; y < board.height(); y++)
      for(int x = 0; x < board.width(); x++)
        if(board.at(x, y).mine) max_ring = std::max(max_ring, gfx::ring_of(board, x, y));
    won = false;
    enter(Scene::EXPLODING);
    return;
  }

  if(board.state() == State::WON) {
    won = true;
    audio::play_sfx(audio::Sfx::CASCADE, 1.5f);
    audio::play_song(audio::Song::WIN);
    gfx::confetti(70);
    enter(Scene::CLEARED);
    return;
  }

  if(r.revealed > 1) {
    audio::play_sfx(audio::Sfx::CASCADE, 0.9f + std::min(r.revealed, 60) / 80.0f);
    if(r.revealed >= 15) {
      char buf[12];
      snprintf(buf, sizeof(buf), "+%d", r.revealed);
      gfx::popup(gfx::cell_centre(layout, cursor_x, cursor_y) - Point(0, 6), buf, gfx::colors::accent);
    }
  } else {
    audio::play_sfx(audio::Sfx::REVEAL);
  }

  // The music picks up as the board clears.
  int safe_total = board.width() * board.height() - board.mine_count();
  float progress = float(board.revealed_count()) / float(std::max(1, safe_total));
  audio::set_tempo_scale(1.0f - 0.15f * progress);
}

void act_on_number() {
  if(board.can_chord(cursor_x, cursor_y)) {
    audio::play_sfx(audio::Sfx::CHORD);
    on_reveal(board.chord(cursor_x, cursor_y));
  } else {
    audio::play_sfx(audio::Sfx::BLOCKED);
  }
}

void update_playing(uint32_t dt) {
  if(buttons.pressed & (Button::MENU | Button::Y)) {
    audio::play_sfx(audio::Sfx::PAUSE);
    audio::pause_song(true);
    pause_index = P_RESUME;
    enter(Scene::PAUSED);
    return;
  }

  if(board.state() == State::PLAYING) play_ms += dt;

  handle_cursor(dt);

  const Cell &c = board.at(cursor_x, cursor_y);
  if(buttons.pressed & Button::A) {
    if(c.revealed) {
      act_on_number();
    } else if(c.flagged) {
      audio::play_sfx(audio::Sfx::BLOCKED);
    } else {
      on_reveal(board.reveal(cursor_x, cursor_y, blit::random() ^ now_ms));
    }
  } else if(buttons.pressed & Button::B) {
    if(c.revealed) {
      act_on_number();
    } else if(board.toggle_flag(cursor_x, cursor_y)) {
      audio::play_sfx(board.at(cursor_x, cursor_y).flagged ? audio::Sfx::FLAG : audio::Sfx::UNFLAG);
    }
  }
}

void update_paused() {
  if(buttons.pressed & Button::MENU) {
    audio::pause_song(false);
    enter(Scene::PLAYING);
    return;
  }
  menu_nav(pause_index, P_COUNT);
  if(!(buttons.pressed & Button::A)) return;

  audio::play_sfx(audio::Sfx::MENU_SELECT);
  switch(pause_index) {
    case P_RESUME:
      audio::pause_song(false);
      enter(Scene::PLAYING);
      break;
    case P_RESTART:
      start_game();
      break;
    case P_MENU:
      go_to_title();
      break;
  }
}

void update_exploding() {
  // A pop for every ring of mines as they go off.
  uint32_t t = now_ms - lose_start;
  if(t >= gfx::RING_DELAY_MS) {
    int ring = int((t - gfx::RING_DELAY_MS) / gfx::RING_MS);
    while(last_ring < std::min(ring, max_ring)) {
      last_ring++;
      bool any = false;
      for(int y = 0; y < board.height(); y++) {
        for(int x = 0; x < board.width(); x++) {
          const Cell &c = board.at(x, y);
          if(!c.mine || c.flagged || c.exploded || gfx::ring_of(board, x, y) != last_ring) continue;
          gfx::burst(gfx::cell_centre(layout, x, y), Pen(255, 150, 50), 3);
          any = true;
        }
      }
      if(any) audio::play_sfx(audio::Sfx::MINE_POP, 1.0f - std::min(last_ring, 10) * 0.04f);
    }
  }
  uint32_t total = gfx::RING_DELAY_MS + max_ring * gfx::RING_MS + 700;
  if(in_scene() >= std::max<uint32_t>(total, 1400) || (in_scene() > 500 && (buttons.pressed & Button::A))) {
    finish_game();
  }
}

void update_name_entry() {
  char &c = entry_name[entry_cursor];
  if(buttons.pressed & Button::DPAD_UP) {
    c = c >= 'Z' ? 'A' : c + 1;
    audio::play_sfx(audio::Sfx::MENU_MOVE);
  }
  if(buttons.pressed & Button::DPAD_DOWN) {
    c = c <= 'A' ? 'Z' : c - 1;
    audio::play_sfx(audio::Sfx::MENU_MOVE);
  }
  if((buttons.pressed & Button::DPAD_LEFT) && entry_cursor > 0) {
    entry_cursor--;
    audio::play_sfx(audio::Sfx::MENU_MOVE);
  }
  if((buttons.pressed & Button::DPAD_RIGHT) && entry_cursor < 2) {
    entry_cursor++;
    audio::play_sfx(audio::Sfx::MENU_MOVE);
  }
  if((buttons.pressed & Button::B) && entry_cursor > 0) {
    entry_cursor--;
    audio::play_sfx(audio::Sfx::MENU_BACK);
  }
  if(buttons.pressed & Button::A) {
    audio::play_sfx(audio::Sfx::MENU_SELECT);
    if(entry_cursor < 2) {
      entry_cursor++;
      return;
    }
    insert_time(difficulty, new_rank, entry_name, std::max<uint32_t>(1, play_ms));
    memcpy(save.last_name, entry_name, sizeof(entry_name));
    write_save_data();
    enter(Scene::RESULTS);
  }
}

void update_results() {
  if(in_scene() < 400) return; // don't skip the screen with a held button
  if(buttons.pressed & Button::A) {
    audio::play_sfx(audio::Sfx::MENU_SELECT);
    start_game();
  } else if(buttons.pressed & (Button::B | Button::MENU)) {
    audio::play_sfx(audio::Sfx::MENU_BACK);
    go_to_title();
  }
}

// ---------------------------------------------------------------------------
// Render

void render_demo_background() {
  gfx::BoardView v;
  v.now = now_ms;
  v.show_cursor = false;
  v.anim_serial = demo.reveal_serial();
  gfx::draw_board(demo, gfx::layout_for(demo, 8), v);
  gfx::dim_screen(165);
}

void render_title(uint32_t time) {
  render_demo_background();
  gfx::draw_logo(3, time);

  const int top = 46;
  const int row_h = 9;
  for(int i = 0; i < M_COUNT; i++) {
    int y = top + i * row_h;
    bool sel = i == menu_index;
    if(sel) {
      screen.pen = gfx::colors::select;
      screen.rectangle(Rect(12, y - 1, 104, row_h));
      int bob = (time / 220) & 1;
      gfx::draw_flag(Point(3 + bob, y - 1), 8);
    }

    Pen label_col = sel ? gfx::colors::text : Pen(170, 182, 214);
    const char *label = "";
    std::string value;
    switch(i) {
      case M_PLAY:       label = "PLAY"; break;
      case M_DIFFICULTY: label = "LEVEL"; value = difficulties[save.difficulty].name; break;
      case M_MUSIC:      label = "MUSIC"; value = save.music ? "ON" : "OFF"; break;
      case M_SOUND:      label = "SOUND"; value = save.sound ? "ON" : "OFF"; break;
      case M_SCORES:     label = "BEST TIMES"; break;
      case M_HELP:       label = "HOW TO PLAY"; break;
      case M_CREDITS:    label = "CREDITS"; break;
    }
    gfx::text(label, Point(16, y), label_col);
    if(!value.empty()) {
      if(sel) value = "< " + value + " >";
      gfx::text(value, Point(112, y), sel ? gfx::colors::accent : gfx::colors::dim, TextAlign::top_right);
    }
  }

  const DifficultyInfo &d = difficulties[save.difficulty];
  char line[40];
  snprintf(line, sizeof(line), "%dx%d, %d mines", d.width, d.height, d.mines);
  gfx::text(line, Point(64, 111), gfx::colors::dim, TextAlign::top_center);
  uint32_t best = best_time(save.difficulty);
  if(best) {
    char t[12];
    gfx::format_time(t, sizeof(t), best);
    snprintf(line, sizeof(line), "BEST %ss %s", t, save.scores[save.difficulty][0].name);
    gfx::text(line, Point(64, 119), gfx::colors::gold, TextAlign::top_center);
  }
}

void render_time_table(int d, int top, int highlight, uint32_t time) {
  for(int i = 0; i < SCORE_SLOTS; i++) {
    const ScoreEntry &e = save.scores[d][i];
    int y = top + i * 9;
    if(i == highlight) {
      screen.pen = ((time / 250) & 1) ? Pen(200, 160, 40, 120) : Pen(200, 160, 40, 70);
      screen.rectangle(Rect(14, y - 1, 100, 9));
    }
    Pen col = i == 0 ? gfx::colors::gold : gfx::colors::text;
    char buf[12];
    snprintf(buf, sizeof(buf), "%d.", i + 1);
    gfx::text(buf, Point(20, y), gfx::colors::dim);
    if(e.time_ms == 0) {
      gfx::text("---", Point(36, y), gfx::colors::dim);
      continue;
    }
    gfx::text(e.name, Point(36, y), col);
    gfx::format_time(buf, sizeof(buf), e.time_ms);
    std::string s = std::string(buf) + "s";
    gfx::text(s, Point(106, y), col, TextAlign::top_right);
  }
}

void render_scores(uint32_t time) {
  render_demo_background();
  gfx::big_text("BEST TIMES", Point(64, 10), 1, gfx::colors::gold);
  gfx::panel(Rect(6, 20, 116, 88));

  std::string title = std::string("< ") + difficulties[scores_page].name + " >";
  gfx::text(title, Point(64, 25), gfx::colors::accent, TextAlign::top_center);
  render_time_table(scores_page, 37, -1, time);

  const Stats &s = save.stats[scores_page];
  char line[40];
  int pct = s.played ? s.won * 100 / s.played : 0;
  snprintf(line, sizeof(line), "WON %d/%d (%d%%)", s.won, s.played, pct);
  gfx::text(line, Point(64, 85), gfx::colors::dim, TextAlign::top_center);
  snprintf(line, sizeof(line), "STREAK %d  BEST %d", s.streak, s.best_streak);
  gfx::text(line, Point(64, 94), gfx::colors::dim, TextAlign::top_center);
  gfx::text("B: BACK", Point(64, 114), gfx::colors::text, TextAlign::top_center);
}

void render_help() {
  render_demo_background();
  gfx::big_text("HOW TO PLAY", Point(64, 8), 1, gfx::colors::accent);
  gfx::panel(Rect(3, 16, 122, 94));
  const char *const lines[] = {
    "A number shows how",
    "many mines touch it.",
    "",
    "A: reveal  B: flag",
    "Press A on a number",
    "once its mines are",
    "flagged to open the",
    "cells around it.",
    "",
    "First cell is safe.",
    "MENU: pause",
  };
  int y = 20;
  for(const char *l : lines) {
    gfx::text(l, Point(64, y), gfx::colors::text, TextAlign::top_center);
    y += 8;
  }
  gfx::text("B: BACK", Point(64, 114), gfx::colors::text, TextAlign::top_center);
}

void render_credits(uint32_t time) {
  render_demo_background();
  gfx::draw_logo(3, time);
  gfx::panel(Rect(6, 42, 116, 66));
  const char *const lines[] = {
    "Minesweeper v1.0.0",
    "",
    "Game: TeriyakiGod",
    "Original chiptune",
    "made for this game",
    "Made with 32blit",
    "chilichip.eu",
  };
  int y = 47;
  for(const char *l : lines) {
    gfx::text(l, Point(64, y), l == lines[0] ? gfx::colors::accent : gfx::colors::text, TextAlign::top_center);
    y += 8;
  }
  gfx::text("B: BACK", Point(64, 114), gfx::colors::text, TextAlign::top_center);
}

gfx::Face current_face() {
  if(board.state() == State::LOST) return gfx::Face::DEAD;
  if(board.state() == State::WON) return gfx::Face::COOL;
  if(scene == Scene::PLAYING && (buttons.state & Button::A)) return gfx::Face::OH;
  return gfx::Face::SMILE;
}

void render_game_world(bool hide_all = false, bool cursor = true) {
  gfx::draw_header(board.mines_left(), play_ms, current_face(), now_ms);
  gfx::BoardView v;
  v.now = now_ms;
  v.anim_serial = anim_serial;
  v.anim_start = anim_start;
  v.losing = board.state() == State::LOST;
  v.lose_start = lose_start;
  v.hide_all = hide_all;
  v.show_cursor = cursor && !board.finished();
  v.cursor_pressed = scene == Scene::PLAYING && (buttons.state & Button::A);
  v.cursor_x = cursor_x;
  v.cursor_y = cursor_y;
  gfx::draw_board(board, layout, v);
  gfx::draw_effects();
}

void render_paused(uint32_t time) {
  render_game_world(true, false);
  gfx::dim_screen(140);
  gfx::panel(Rect(24, 36, 80, 58));
  gfx::big_text("PAUSED", Point(64, 47), 2, gfx::colors::text);
  const char *const items[] = {"RESUME", "RESTART", "MENU"};
  for(int i = 0; i < P_COUNT; i++) {
    int y = 61 + i * 10;
    bool sel = i == pause_index;
    if(sel) {
      screen.pen = gfx::colors::select;
      screen.rectangle(Rect(30, y - 1, 68, 9));
    }
    gfx::text(items[i], Point(64, y), sel ? gfx::colors::text : gfx::colors::dim, TextAlign::top_center);
  }
}

void render_cleared(uint32_t time) {
  render_game_world();
  uint32_t t = in_scene();
  int scale = t < 150 ? 1 + int(t / 50) : 3;
  if(scale > 3) scale = 3;
  Pen c = ((time / 160) & 1) ? gfx::colors::gold : gfx::colors::text;
  gfx::big_text("CLEAR!", Point(64, 60), scale, c);
  char buf[16];
  gfx::format_time(buf, sizeof(buf), play_ms);
  std::string s = std::string(buf) + "s";
  if(t > 300) gfx::big_text(s, Point(64, 82), 2, gfx::colors::text);
}

void render_name_entry(uint32_t time) {
  render_game_world();
  gfx::dim_screen(150);
  gfx::panel(Rect(10, 20, 108, 88));

  Pen title = ((time / 250) & 1) ? gfx::colors::gold : gfx::colors::text;
  gfx::big_text(new_rank == 0 ? "NEW BEST!" : "TOP 5 TIME!", Point(64, 31), new_rank == 0 ? 2 : 1, title);

  char t[12], line[40];
  gfx::format_time(t, sizeof(t), play_ms);
  snprintf(line, sizeof(line), "#%d   %ss   %s", new_rank + 1, t, info().name);
  gfx::text(line, Point(64, 43), gfx::colors::text, TextAlign::top_center);
  gfx::text("ENTER YOUR NAME", Point(64, 54), gfx::colors::dim, TextAlign::top_center);

  for(int i = 0; i < 3; i++) {
    int cx = 46 + i * 18;
    bool sel = i == entry_cursor;
    char ch[2] = {entry_name[i], 0};
    gfx::big_text(ch, Point(cx, 76), 2, sel ? gfx::colors::accent : gfx::colors::text);
    if(sel) {
      int bob = (time / 200) & 1;
      screen.pen = gfx::colors::accent;
      screen.triangle(Point(cx - 3, 67 - bob), Point(cx + 3, 67 - bob), Point(cx, 64 - bob));
      screen.triangle(Point(cx - 3, 85 + bob), Point(cx + 3, 85 + bob), Point(cx, 88 + bob));
    }
  }
  gfx::text("A: OK   B: BACK", Point(64, 96), gfx::colors::dim, TextAlign::top_center);
}

void render_results(uint32_t time) {
  render_game_world();
  char line[40], t[12];

  if(!won) {
    // Keep the board in view so the player can see what went wrong.
    gfx::panel(Rect(8, 96, 112, 30));
    gfx::big_text("BOOM!", Point(40, 106), 2, gfx::colors::red);
    int safe_total = board.width() * board.height() - board.mine_count();
    snprintf(line, sizeof(line), "%d%% CLEAR", board.revealed_count() * 100 / std::max(1, safe_total));
    gfx::text(line, Point(112, 101), gfx::colors::text, TextAlign::top_right);
    if(in_scene() >= 400) gfx::text("A: RETRY  B: MENU", Point(64, 116), gfx::colors::dim, TextAlign::top_center);
    return;
  }

  gfx::dim_screen(150);
  gfx::panel(Rect(6, 10, 116, 110));
  gfx::big_text("CLEARED!", Point(64, 20), 2, gfx::colors::gold);

  gfx::format_time(t, sizeof(t), play_ms);
  snprintf(line, sizeof(line), "%s IN %ss", info().name, t);
  gfx::text(line, Point(64, 32), gfx::colors::text, TextAlign::top_center);

  if(new_rank == 0) {
    Pen c = ((time / 250) & 1) ? gfx::colors::gold : gfx::colors::text;
    gfx::text("NEW BEST TIME!", Point(64, 42), c, TextAlign::top_center);
  } else if(new_rank > 0) {
    snprintf(line, sizeof(line), "RANK #%d", new_rank + 1);
    gfx::text(line, Point(64, 42), gfx::colors::gold, TextAlign::top_center);
  } else if(best_at_start) {
    gfx::format_time(t, sizeof(t), best_at_start);
    snprintf(line, sizeof(line), "BEST IS %ss", t);
    gfx::text(line, Point(64, 42), gfx::colors::dim, TextAlign::top_center);
  }

  screen.pen = Pen(90, 120, 200, 120);
  screen.rectangle(Rect(14, 52, 100, 1));
  render_time_table(difficulty, 56, new_rank, time);

  if(in_scene() >= 400) gfx::text("A: AGAIN    B: MENU", Point(64, 108), gfx::colors::text, TextAlign::top_center);
}

}

// ---------------------------------------------------------------------------

void init() {
  set_screen_mode(ScreenMode::hires);
  screen.sprites = Surface::load(asset_sprites);
  load_save();
  audio::init();
  apply_audio_settings();
  menu_index = M_PLAY;
  go_to_title();
}

void update(uint32_t time) {
  uint32_t dt = last_update ? time - last_update : 10;
  last_update = time;
  now_ms = time;
  if(dt > 100) dt = 100;

  audio::update(time);
  gfx::update_effects(dt);

  switch(scene) {
    case Scene::TITLE:
      update_demo(dt);
      update_title();
      break;
    case Scene::SCORES:
    case Scene::HELP:
    case Scene::CREDITS:
      update_demo(dt);
      update_info_screen();
      break;
    case Scene::PLAYING:
      update_playing(dt);
      break;
    case Scene::PAUSED:
      update_paused();
      break;
    case Scene::EXPLODING:
      update_exploding();
      break;
    case Scene::CLEARED:
      if(in_scene() >= CLEARED_MS) finish_game();
      break;
    case Scene::NAME_ENTRY:
      update_name_entry();
      break;
    case Scene::RESULTS:
      update_results();
      break;
  }
}

void render(uint32_t time) {
  screen.alpha = 255;
  screen.pen = gfx::colors::bg;
  screen.clear();

  if(now_ms < shake_until) {
    int mag = (shake_until - now_ms) / 150 + 1;
    gfx::set_shake(Point(int(blit::random() % (mag * 2 + 1)) - mag, int(blit::random() % (mag * 2 + 1)) - mag));
  } else {
    gfx::set_shake(Point(0, 0));
  }

  switch(scene) {
    case Scene::TITLE:      render_title(time); break;
    case Scene::SCORES:     render_scores(time); break;
    case Scene::HELP:       render_help(); break;
    case Scene::CREDITS:    render_credits(time); break;
    case Scene::PLAYING:    render_game_world(); break;
    case Scene::PAUSED:     render_paused(time); break;
    case Scene::EXPLODING:  render_game_world(); break;
    case Scene::CLEARED:    render_cleared(time); break;
    case Scene::NAME_ENTRY: render_name_entry(time); break;
    case Scene::RESULTS:    render_results(time); break;
  }

  gfx::set_shake(Point(0, 0));
  if(now_ms < flash_until) {
    screen.pen = Pen(255, 230, 200, uint8_t((flash_until - now_ms) * 200 / 160));
    screen.rectangle(Rect(0, 0, 128, 128));
  }
}
