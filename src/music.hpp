#ifndef MUSIC_HPP
#define MUSIC_HPP

#include "32blit.hpp"
#include <vector>
#include <cstdint>
#include <sstream>  // CRITICAL: Fixes the "incomplete type" error
#include <cmath>    // CRITICAL: For the pow() function

// --- Frequency Definitions (Standard Pitch A4 = 440Hz) ---
#define REST    0
// Octave 2
#define NOTE_C2  65
#define NOTE_D2  73
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_G2  98
#define NOTE_A2  110
#define NOTE_B2  123
// Octave 3
#define NOTE_C3  131
#define NOTE_D3  147
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247
// Octave 4
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
// Octave 5
#define NOTE_C5  523
#define NOTE_CS5  554
#define NOTE_D5  587
#define NOTE_DS5  622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
// Octave 6
#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_G6  1568
#define NOTE_A6  1760
#define NOTE_B6  1976

// Timing Definitions (120 BPM)
#define SN 125  // Sixteenth Note
#define EN 250  // Eighth Note
#define QN 500  // Quarter Note
#define HN 1000 // Half Note
#define WN 2000 // Whole Note

// Note structure
struct Note {
    uint32_t freq;
    uint32_t dur_ms;
};

// Individual Track Class
class Music {
public:
    Music(const Note* melody, size_t count);
    Music(const std::string& rtttl);

    // Configuration
    void set_adsr(uint16_t a, uint16_t d, uint16_t s, uint16_t r);
    void set_waveform(blit::Waveform wave) { m_waveform = wave; }
    void set_volume(uint16_t volume) { m_volume = volume; }
    void set_speed(float multiplier) { m_speed = multiplier; }
    
    uint16_t get_volume() const { return m_volume; }

    // Controls
    void play() { m_playing = true; }
    void pause() { m_playing = false; }
    void stop();

    // Internal Logic
    void update(uint32_t dt, blit::AudioChannel& channel);

private:
    std::vector<Note> m_dynamic_melody;
    const Note* m_melody;
    size_t m_count;
    size_t m_index = 0;
    uint32_t m_timer = 0;
    bool m_playing = false;
    float m_speed = 1.0f;

    uint16_t m_attack = 2, m_decay = 200, m_sustain = 0xFFFF, m_release = 2, m_volume = 100;
    blit::Waveform m_waveform = blit::Waveform::SQUARE;
};

// Hardware Conductor Class
class MusicManager {
public:
    MusicManager() = default;

    void add_track(uint8_t channel_idx, Music* music_ptr);
    void update(uint32_t dt);
    
    void play_all();
    void stop_all();
    
    // Volume management
    void set_track_volume(uint8_t channel_idx, uint16_t volume);
    void set_master_volume(float multiplier); // 0.0 to 1.0

private:
    struct ManagedTrack {
        Music* music;
        uint8_t channel_idx;
    };
    std::vector<ManagedTrack> m_tracks;
};

#endif