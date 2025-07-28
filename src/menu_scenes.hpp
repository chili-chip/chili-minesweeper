#pragma once

#include "32blit.hpp"
#include "scene.hpp"
#include "minesweeper.hpp"
#include "input.hpp"
#include "assets.hpp"
#include "timer.hpp"
#include <string>
#include <vector>

// Forward declarations
class GameScene;

class MenuScene : public Scene {
public:
    void enter();
    void update();
    void render();

private:
    uint8_t option_selected;
    
    const std::vector<std::string> MENU_OPTIONS{
        "Play Game",
        "Credits"
    };
};

class CreditsScene : public Scene {
public:
    void update();
    void render();

private:
    const std::vector<std::string> CREDITS_TEXT{
        "MINESWEEPER",
        "",
        "Classic game",
        "built with 32blit",
        "",
        "Controls:",
        "D-pad: Move",
        "A: Reveal",
        "B: Flag",
        "X: Reset",
        "",
        "Press A to return"
    };
};

class GameScene : public Scene {
public:
    GameScene(int width, int height, int mines);
    ~GameScene();
    
    void enter();
    void leave();
    void update();
    void render();

private:
    Minesweeper* board;
    Input* input;
    int board_width, board_height, mine_count;
};
