#include "save.hpp"

#include "32blit.hpp"

#include <cstring>

using namespace blit;

namespace ms {

// Board sizes are picked to fill the 128x128 screen under the 8px header.
const DifficultyInfo difficulties[DIFFICULTY_COUNT] = {
  {"EASY", 9, 9, 10, 13},
  {"MEDIUM", 12, 12, 24, 10},
  {"HARD", 16, 15, 40, 8},
  {"EXPERT", 16, 15, 50, 8},
};

static const uint32_t SAVE_MAGIC = 0x314d5357; // "WSM1"
static const uint8_t SAVE_VERSION = 1;

SaveData save;

static void set_defaults() {
  memset(&save, 0, sizeof(save));
  save.magic = SAVE_MAGIC;
  save.version = SAVE_VERSION;
  save.difficulty = MEDIUM;
  save.music = 1;
  save.sound = 1;
  strcpy(save.last_name, "AAA");
}

void load_save() {
  if(read_save(save) && save.magic == SAVE_MAGIC && save.version == SAVE_VERSION) {
    for(auto &table : save.scores)
      for(auto &e : table) e.name[3] = 0;
    save.last_name[3] = 0;
    if(save.difficulty >= DIFFICULTY_COUNT) save.difficulty = MEDIUM;
    return;
  }
  set_defaults();
  write_save_data();
}

void write_save_data() {
  write_save(save);
}

int time_rank(int difficulty, uint32_t time_ms) {
  if(difficulty < 0 || difficulty >= DIFFICULTY_COUNT || time_ms == 0) return -1;
  for(int i = 0; i < SCORE_SLOTS; i++) {
    const ScoreEntry &e = save.scores[difficulty][i];
    if(e.time_ms == 0 || time_ms < e.time_ms) return i;
  }
  return -1;
}

void insert_time(int difficulty, int rank, const char *name, uint32_t time_ms) {
  if(difficulty < 0 || difficulty >= DIFFICULTY_COUNT || rank < 0 || rank >= SCORE_SLOTS) return;
  ScoreEntry *table = save.scores[difficulty];
  for(int i = SCORE_SLOTS - 1; i > rank; i--) table[i] = table[i - 1];
  ScoreEntry &e = table[rank];
  memset(&e, 0, sizeof(e));
  memcpy(e.name, name, 3);
  e.time_ms = time_ms;
}

void record_game(int difficulty, bool won) {
  if(difficulty < 0 || difficulty >= DIFFICULTY_COUNT) return;
  Stats &s = save.stats[difficulty];
  if(s.played < 0xffff) s.played++;
  if(won) {
    if(s.won < 0xffff) s.won++;
    if(s.streak < 0xffff) s.streak++;
    if(s.streak > s.best_streak) s.best_streak = s.streak;
  } else {
    s.streak = 0;
  }
}

}
