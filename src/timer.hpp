#ifndef TIMER_HPP
#define TIMER_HPP

#include "32blit.hpp"

class GameTimer {
private:
    static blit::Timer timer;
    static int time;
    static void increase_time(blit::Timer &timer);
public:
    static void init();
    static void start();
    static void pause();
    static void reset();
    static int get_seconds();
};

#endif // TIMER_HPP