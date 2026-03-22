#include "game.hpp"
#include "music.hpp"

using namespace blit;

// A "Thinker's" melody in A-Minor
static const Note minesweeper_theme[] = {
    {NOTE_A4, EN}, {NOTE_E5, EN}, {NOTE_A4, EN}, {NOTE_E5, EN},
    {NOTE_G4, EN}, {NOTE_D5, EN}, {NOTE_G4, EN}, {NOTE_D5, EN},
    {NOTE_F4, EN}, {NOTE_C5, EN}, {NOTE_F4, EN}, {NOTE_C5, EN},
    {NOTE_E4, QN}, {NOTE_B4, QN}, {REST,    QN},
    
    // Variation bridge
    {NOTE_A4, SN}, {NOTE_B4, SN}, {NOTE_C5, EN}, {NOTE_B4, EN}, {NOTE_A4, EN},
    {NOTE_G4, QN}, {NOTE_E4, QN}, {REST,    QN}
};

Music bgm(minesweeper_theme, sizeof(minesweeper_theme) / sizeof(Note));
MusicManager audioSystem;

// Global scene pointer - this is the only global variable we need
Scene* current_scene = nullptr;

void init() {
    set_screen_mode(ScreenMode::hires);
    screen.sprites = Surface::load(asset_sprites);
    
    // Start with the menu scene
    current_scene = new MenuScene();
    current_scene->enter();

    // Voice setup: Short attack and long decay for a "plinky" sound
    bgm.set_adsr(10, 50, 0x2000, 50);
    bgm.set_waveform(blit::Waveform::SQUARE);
    bgm.set_volume(100);

    // Add to channel 0
    audioSystem.add_track(0, &bgm);
    
    // Start the background loop
    bgm.play();
}

void update(uint32_t time) {
    static uint32_t last_time = 0;
    uint32_t dt = time - last_time;
    last_time = time;
    if (current_scene) {
        current_scene->update();
        
        // Check if we need to change scenes
        if (current_scene->finished()) {
            Scene* next_scene = current_scene->next();
            
            // Clean up the old scene
            current_scene->leave();
            delete current_scene;
            
            // Change to the new scene
            current_scene = next_scene;
            if (current_scene) {
                current_scene->enter();
            }
        }
    }
    audioSystem.update(dt);
}

void render(uint32_t time) {
    if (current_scene) {
        current_scene->render();
    }
}
