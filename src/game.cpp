#include "game.hpp"

using namespace blit;

// Global scene pointer - this is the only global variable we need
Scene* current_scene = nullptr;

void init() {
    set_screen_mode(ScreenMode::hires);
    screen.sprites = Surface::load(asset_sprites);
    
    // Start with the menu scene
    current_scene = new MenuScene();
    current_scene->enter();
}

void update(uint32_t time) {
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
}

void render(uint32_t time) {
    if (current_scene) {
        current_scene->render();
    }
}
