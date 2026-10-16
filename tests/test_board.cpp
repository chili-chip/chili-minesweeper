// Unit tests for the board rules in src/board.cpp. No framework: each test is
// a function, CHECK records failures and main() returns non-zero if any fail.
#include "board.hpp"

#include <cstdio>
#include <set>
#include <utility>

using namespace ms;

static int failures = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    if(!(cond)) {                                                       \
      std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                                       \
    }                                                                   \
  } while(0)

static int count_mines(const Board &b) {
  int n = 0;
  for(int y = 0; y < b.height(); y++)
    for(int x = 0; x < b.width(); x++)
      if(b.at(x, y).mine) n++;
  return n;
}

static void test_first_reveal_is_safe_and_opens_an_area() {
  for(uint32_t seed = 1; seed < 200; seed++) {
    Board b;
    b.reset(16, 15, 50);
    RevealResult r = b.reveal(7, 7, seed);
    CHECK(!r.hit_mine);
    CHECK(b.state() == State::PLAYING || b.state() == State::WON);
    CHECK(count_mines(b) == 50);
    // The whole 3x3 block around the first reveal is mine free, so the first
    // cell is always a zero and opens an area.
    for(int dy = -1; dy <= 1; dy++)
      for(int dx = -1; dx <= 1; dx++) CHECK(!b.at(7 + dx, 7 + dy).mine);
    CHECK(b.at(7, 7).adjacent == 0);
    CHECK(r.revealed >= 9);
  }
}

static void test_mine_count_capped_when_board_is_tiny() {
  Board b;
  b.reset(3, 3, 20);
  CHECK(b.mine_count() == 8);
  RevealResult r = b.reveal(1, 1, 42);
  CHECK(!r.hit_mine);
  CHECK(count_mines(b) == 8);
  CHECK(b.state() == State::WON);
}

static void test_same_seed_same_board() {
  Board a, b;
  a.reset(9, 9, 10);
  b.reset(9, 9, 10);
  a.reveal(0, 0, 1234);
  b.reveal(0, 0, 1234);
  for(int y = 0; y < 9; y++)
    for(int x = 0; x < 9; x++) CHECK(a.at(x, y).mine == b.at(x, y).mine);
}

// 4x3 board with mines at (3,0) and (3,2):
//   . . 1 *
//   . . 2 2      <- (3,1) is a 2 as well
//   . . 1 *
static Board fixed_board() {
  Board b;
  b.reset(4, 3, 2);
  b.set_mines({{3, 0}, {3, 2}});
  return b;
}

static void test_adjacent_counts() {
  Board b = fixed_board();
  CHECK(b.at(0, 0).adjacent == 0);
  CHECK(b.at(2, 0).adjacent == 1);
  CHECK(b.at(2, 1).adjacent == 2);
  CHECK(b.at(3, 1).adjacent == 2);
  CHECK(b.at(2, 2).adjacent == 1);
}

static void test_flood_fill_stops_at_numbers_and_records_waves() {
  Board b = fixed_board();
  RevealResult r = b.reveal(0, 0, 0);
  CHECK(!r.hit_mine);
  // Columns 0-2 open; column 3 (two mines and a 2) stays hidden.
  CHECK(r.revealed == 9);
  CHECK(!b.at(3, 1).revealed);
  CHECK(b.at(0, 0).wave == 0);
  CHECK(b.at(1, 1).wave == 1);
  CHECK(b.at(2, 2).wave == 2);
  CHECK(r.max_wave == 2);
  CHECK(b.state() == State::PLAYING);
  CHECK(b.safe_cells_left() == 1);
}

static void test_win_flags_remaining_mines() {
  Board b = fixed_board();
  b.reveal(0, 0, 0);
  RevealResult r = b.reveal(3, 1, 0);
  CHECK(r.revealed == 1);
  CHECK(b.state() == State::WON);
  CHECK(b.at(3, 0).flagged);
  CHECK(b.at(3, 2).flagged);
  CHECK(b.mines_left() == 0);
}

static void test_hitting_a_mine_loses_and_marks_wrong_flags() {
  Board b = fixed_board();
  b.reveal(0, 0, 0);
  CHECK(b.toggle_flag(3, 1)); // wrong: not a mine
  CHECK(b.mines_left() == 1);
  RevealResult r = b.reveal(3, 0, 0);
  CHECK(r.hit_mine);
  CHECK(b.state() == State::LOST);
  CHECK(b.at(3, 0).exploded);
  CHECK(b.at(3, 2).revealed);
  CHECK(!b.at(3, 2).exploded);
  CHECK(b.at(3, 1).wrong_flag);
  CHECK(b.exploded_x() == 3 && b.exploded_y() == 0);
}

static void test_flags_block_reveal_and_cannot_go_on_revealed_cells() {
  Board b = fixed_board();
  b.reveal(0, 0, 0);
  CHECK(!b.toggle_flag(0, 0)); // revealed
  CHECK(b.toggle_flag(3, 0));
  CHECK(b.at(3, 0).flagged);
  RevealResult r = b.reveal(3, 0, 0);
  CHECK(r.revealed == 0);
  CHECK(b.state() == State::PLAYING);
  CHECK(b.toggle_flag(3, 0));
  CHECK(!b.at(3, 0).flagged);
}

static void test_chord_reveals_neighbours_when_flags_match() {
  Board b = fixed_board();
  b.reveal(0, 0, 0);
  // (2,0) is a 1 touching the mine at (3,0). Not flagged yet: chord does nothing.
  CHECK(!b.can_chord(2, 0));
  CHECK(b.chord(2, 0).revealed == 0);
  b.toggle_flag(3, 0);
  CHECK(b.can_chord(2, 0));
  RevealResult r = b.chord(2, 0);
  CHECK(r.revealed == 1); // (3,1)
  CHECK(!r.hit_mine);
  CHECK(b.state() == State::WON);
}

static void test_chord_with_wrong_flag_explodes() {
  Board b = fixed_board();
  b.reveal(0, 0, 0);
  b.toggle_flag(3, 1); // wrong flag next to (2,0)
  RevealResult r = b.chord(2, 0);
  CHECK(r.hit_mine);
  CHECK(b.state() == State::LOST);
  CHECK(b.at(3, 0).exploded);
}

static void test_no_actions_after_game_ends() {
  Board b = fixed_board();
  b.reveal(0, 0, 0);
  b.reveal(3, 0, 0);
  CHECK(b.state() == State::LOST);
  CHECK(b.reveal(3, 1, 0).revealed == 0);
  CHECK(!b.toggle_flag(3, 1));
}

static void test_out_of_bounds_is_ignored() {
  Board b;
  b.reset(9, 9, 10);
  CHECK(b.reveal(-1, 0, 1).revealed == 0);
  CHECK(b.reveal(9, 0, 1).revealed == 0);
  CHECK(!b.toggle_flag(0, 9));
  CHECK(b.state() == State::READY);
}

static void test_flagging_before_first_reveal_keeps_flags() {
  Board b;
  b.reset(9, 9, 10);
  CHECK(b.toggle_flag(8, 8));
  b.reveal(0, 0, 77);
  CHECK(b.at(8, 8).flagged);
  CHECK(!b.at(8, 8).revealed);
  CHECK(count_mines(b) == 10);
}

int main() {
  test_first_reveal_is_safe_and_opens_an_area();
  test_mine_count_capped_when_board_is_tiny();
  test_same_seed_same_board();
  test_adjacent_counts();
  test_flood_fill_stops_at_numbers_and_records_waves();
  test_win_flags_remaining_mines();
  test_hitting_a_mine_loses_and_marks_wrong_flags();
  test_flags_block_reveal_and_cannot_go_on_revealed_cells();
  test_chord_reveals_neighbours_when_flags_match();
  test_chord_with_wrong_flag_explodes();
  test_no_actions_after_game_ends();
  test_out_of_bounds_is_ignored();
  test_flagging_before_first_reveal_keeps_flags();

  if(failures) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("all board tests passed\n");
  return 0;
}
