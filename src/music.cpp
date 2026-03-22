#include "music.hpp"

using namespace blit;

// --- Music Implementation ---

Music::Music(const Note* melody, size_t count) 
    : m_melody(melody), m_count(count) {}

Music::Music(const std::string& rtttl) {
    size_t pos = 0;

    // 1. Skip the Name (e.g., "MissionImp:")
    pos = rtttl.find(':');
    if (pos == std::string::npos) return;
    pos++;

    // 2. Parse Defaults (e.g., "d=4,o=5,b=125:")
    int default_dur = 4;
    int default_oct = 5;
    int bpm = 63;

    size_t end_defaults = rtttl.find(':', pos);
    std::string defaults = rtttl.substr(pos, end_defaults - pos);
    
    if (defaults.find("d=") != std::string::npos) 
        default_dur = std::stoi(defaults.substr(defaults.find("d=") + 2));
    if (defaults.find("o=") != std::string::npos) 
        default_oct = std::stoi(defaults.substr(defaults.find("o=") + 2));
    if (defaults.find("b=") != std::string::npos) 
        bpm = std::stoi(defaults.substr(defaults.find("b=") + 2));

    pos = end_defaults + 1;

    // Calculate timing based on BPM
    // Wholenote ms = (60000 / BPM) * 4
    uint32_t wholenote = (60000 * 4) / bpm;

    // 3. Parse Note Data (e.g., "8a#,16c6")
    std::string data = rtttl.substr(pos);
    std::stringstream ss(data);
    std::string segment;

    while (std::getline(ss, segment, ',')) {
        if (segment.empty()) continue;
        
        size_t i = 0;
        // Get Duration
        int duration = default_dur;
        if (isdigit(segment[i])) {
            std::string d_str;
            while(isdigit(segment[i])) d_str += segment[i++];
            duration = std::stoi(d_str);
        }

        // Get Note Name
        char note_char = tolower(segment[i++]);
        bool sharp = false;
        if (segment[i] == '#') { sharp = true; i++; }

        // Get Dot (1.5x duration)
        bool dotted = false;
        if (segment[i] == '.') { dotted = true; i++; }

        // Get Octave
        int octave = default_oct;
        if (isdigit(segment[i])) {
            octave = segment[i++] - '0';
        }

        // Convert to Frequency
        uint32_t freq = 0;
        if (note_char != 'p') { // 'p' is a pause/rest
            // Frequency Map (Octave 0 base)
            static const float notes[] = { 16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50, 25.96, 27.50, 29.14, 30.87 };
            int note_idx = 0;
            switch(note_char) {
                case 'c': note_idx = 0; break;
                case 'd': note_idx = 2; break;
                case 'e': note_idx = 4; break;
                case 'f': note_idx = 5; break;
                case 'g': note_idx = 7; break;
                case 'a': note_idx = 9; break;
                case 'b': note_idx = 11; break;
            }
            if (sharp) note_idx++;
            
            // Formula: Freq = Base * 2^Octave
            freq = static_cast<uint32_t>(notes[note_idx] * pow(2, octave));
        }

        uint32_t dur_ms = wholenote / duration;
        if (dotted) dur_ms += (dur_ms / 2);

        m_dynamic_melody.push_back({freq, dur_ms});
    }

    // Point the existing melody pointer to our new vector
    m_melody = m_dynamic_melody.data();
    m_count = m_dynamic_melody.size();
}

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