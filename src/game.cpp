#include "game.hpp"
#include "music.hpp"

using namespace blit;

std::string bg_theme = "jamesbond:d=4,o=6,b=112:16c.5,32d.5,32d.5,16d.5,8d.5,16c.5,16c.5,16c.5,16c.5,32d#.5,32d#.5,16d#.5,8d#.5,16d.5,16d.5,16d.5,16c.5,32d.5,32d.5,16d.5,8d.5,16c.5,16c.5,16c.5,16c.5,32d#.5,32d#.5,16d#.5,8d#.5,16d.5,16d.5,16d.5,16c.5,32d.5,32d.5,16d.5,8d.5,16c.5,16c.5,16c.5,16c.5,32d#.5,32d#.5,16d#.5,8d#.5,16d.5,16d.5,16c.5,16b.5,2a#5,16g.5,16f.5,2g.5";
Music bgm(bg_theme);
MusicManager audioSystem;

// Global scene pointer - this is the only global variable we need
Scene* current_scene = nullptr;

void init() {
    set_screen_mode(ScreenMode::hires);
    screen.sprites = Surface::load(asset_sprites);
    
    // Start with the menu scene
    current_scene = new MenuScene();
    current_scene->enter();

    bgm.set_waveform(blit::Waveform::SQUARE);
    bgm.set_volume(3000);
    audioSystem.add_track(0, &bgm);
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
