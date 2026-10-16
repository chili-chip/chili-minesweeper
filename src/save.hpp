#pragma once

#include <cstdint>

// Settings, best times and stats, persisted with the 32blit save API.
namespace ms {

enum Difficulty : uint8_t { EASY, MEDIUM, HARD, EXPERT, DIFFICULTY_COUNT };

struct DifficultyInfo {
  const char *name;
  int width, height, mines;
  int cell; // cell size in pixels
};

extern const DifficultyInfo difficulties[DIFFICULTY_COUNT];

constexpr int SCORE_SLOTS = 5;

struct ScoreEntry {
  char name[4];     // three letters + terminator
  uint32_t time_ms; // 0 = empty slot
};

struct Stats {
  uint16_t played;
  uint16_t won;
  uint16_t streak;
  uint16_t best_streak;
};

struct SaveData {
  uint32_t magic;
  uint8_t version;
  uint8_t difficulty;
  uint8_t music;
  uint8_t sound;
  char last_name[4];
  ScoreEntry scores[DIFFICULTY_COUNT][SCORE_SLOTS];
  Stats stats[DIFFICULTY_COUNT];
};

extern SaveData save;

void load_save();
void write_save_data();

// Rank (0-based) a winning time would take for the difficulty, or -1.
int time_rank(int difficulty, uint32_t time_ms);
void insert_time(int difficulty, int rank, const char *name, uint32_t time_ms);

void record_game(int difficulty, bool won);

}
