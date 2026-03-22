#include "music.hpp"

using namespace blit;

// --- Music Implementation ---

Music::Music(const Note* melody, size_t count) 
    : m_melody(melody), m_count(count) {}

void Music::set_adsr(uint16_t a, uint16_t d, uint16_t s, uint16_t r) {
    m_attack = a; m_decay = d; m_sustain = s; m_release = r;
}

void Music::stop() {
    m_playing = false;
    m_index = 0;
    m_timer = 0;
}

void Music::update(uint32_t dt, AudioChannel& channel) {
    if (!m_playing || m_count == 0) return;

    if (m_timer > dt) {
        m_timer -= dt;
    } else {
        channel.off();

        const Note& n = m_melody[m_index];

        // Apply settings to the 32blit channel
        channel.waveforms = m_waveform;
        channel.attack_ms = m_attack;
        channel.decay_ms = m_decay;
        channel.sustain = m_sustain;
        channel.release_ms = m_release;
        channel.volume = m_volume;

        if (n.freq > 0) {
            channel.frequency = n.freq;
            channel.trigger_attack();
        }

        m_timer = static_cast<uint32_t>(n.dur_ms / m_speed);
        m_index = (m_index + 1) % m_count;
    }
}

// --- MusicManager Implementation ---

void MusicManager::add_track(uint8_t channel_idx, Music* music_ptr) {
    m_tracks.push_back({music_ptr, channel_idx});
}

void MusicManager::update(uint32_t dt) {
    for (auto& track : m_tracks) {
        track.music->update(dt, channels[track.channel_idx]);
    }
}

void MusicManager::play_all() {
    for (auto& t : m_tracks) t.music->play();
}

void MusicManager::stop_all() {
    for (auto& t : m_tracks) {
        t.music->stop();
        channels[t.channel_idx].off();
    }
}

void MusicManager::set_track_volume(uint8_t channel_idx, uint16_t volume) {
    for (auto &t : m_tracks) {
        if (t.channel_idx == channel_idx) {
            t.music->set_volume(volume);
        }
    }
}

void MusicManager::set_master_volume(float multiplier) {
    for (auto &t : m_tracks) {
        uint16_t v = t.music->get_volume();
        t.music->set_volume(static_cast<uint16_t>(v * multiplier));
    }
}