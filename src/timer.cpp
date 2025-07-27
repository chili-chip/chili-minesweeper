#include "timer.hpp"

blit::Timer GameTimer::timer;
int GameTimer::time = 0;

void GameTimer::increase_time(blit::Timer &timer) {
    time++;
}

void GameTimer::init() {
    timer.init(increase_time, 1000);
}

void GameTimer::start() {
    timer.start();
}

void GameTimer::pause() {
    timer.stop();
}

void GameTimer::reset() {
    timer.stop();
    time = 0;
}

int GameTimer::get_seconds() {
    return time;
}