#include "menu_scenes.hpp"
#include "config.hpp"

using namespace blit;

// MenuScene implementation
void MenuScene::enter() {
    option_selected = 0;
}

void MenuScene::update() {
    if (buttons.pressed & Button::DPAD_UP) {
        option_selected = (option_selected - 1) % MENU_OPTIONS.size();
    }
    else if (buttons.pressed & Button::DPAD_DOWN) {
        option_selected = (option_selected + 1) % MENU_OPTIONS.size();
    }

    if (buttons.released & Button::A) {
        switch (option_selected) {
            case 0: // Game
                finish(new GameScene(board_width, board_height, 40));
                break;
            case 1: // Credits
                finish(new CreditsScene());
                break;
        }
    }
}

void MenuScene::render() {
    // Fill screen with dark blue background
    screen.pen = Pen(20, 20, 40);
    screen.clear();
    
    screen.pen = Pen(255, 255, 255);
    
    // Draw title
    screen.text("MINESWEEPER", minimal_font, Point(64, 15), true, center_center);
    
    // Draw menu options
    for (uint8_t i = 0; i < MENU_OPTIONS.size(); i++) {
        Point pos(15, 35 + 15 * i);
        screen.text(MENU_OPTIONS[i], minimal_font, pos);
        
        // Draw selection indicator
        if (i == option_selected) {
            screen.rectangle(Rect(8, 35 + 15 * i, 3, 6));
        }
    }
    
    // Draw difficulty info for selected option
    screen.pen = Pen(200, 200, 200);
    switch(option_selected) {
        case 0: screen.text("16x16, 40 mines", minimal_font, Point(64, 100), true, center_center); break;
        case 1: screen.text("Game info", minimal_font, Point(64, 100), true, center_center); break;
    }
    
    // Simple instruction
    screen.pen = Pen(150, 150, 150);
    screen.text("A: Select", minimal_font, Point(64, 115), true, center_center);
}

// CreditsScene implementation
void CreditsScene::update() {
    if (buttons.released & Button::A) {
        finish(new MenuScene());
    }
}

void CreditsScene::render() {
    // Fill screen with dark blue background
    screen.pen = Pen(20, 20, 40);
    screen.clear();
    
    screen.pen = Pen(255, 255, 255);
    
    // Draw credits text
    for (uint8_t i = 0; i < CREDITS_TEXT.size(); i++) {
        Point pos(64, 8 + 9 * i);
        screen.text(CREDITS_TEXT[i], minimal_font, pos, true, center_center);
    }
}

// GameScene implementation
GameScene::GameScene(int width, int height, int mines) 
    : board_width(width), board_height(height), mine_count(mines) {
    board = new Minesweeper(width, height, mines);
    input = new Input(*board);
}

GameScene::~GameScene() {
    delete board;
    delete input;
}

void GameScene::enter() {
    GameTimer::init();
}

void GameScene::leave() {
    // Nothing special needed here for now
}

void GameScene::update() {
    if (!board->is_game_over() && !board->is_win() && GameTimer::get_seconds() < game_time_limit) {
        input->handle_cursor_movement();
        input->handle_cell_actions();
    } else {
        GameTimer::pause();
        input->handle_game_reset();
    }
    
    // Allow returning to menu with Menu button or after game ends
    if (buttons.released & Button::MENU || 
        (board->is_game_over() && buttons.released & Button::B) ||
        (board->is_win() && buttons.released & Button::B)) {
        finish(new MenuScene());
    }
}

void GameScene::render() {
    screen.pen = Pen(20, 20, 40);  // Dark blue background
    screen.clear();
    board->draw();
    input->cursor.draw();
}
