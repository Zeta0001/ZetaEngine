#pragma once
#include <print>
#include <vector>

#include <Zeta/time.hpp>
#include <Zeta/window.hpp>
#include <Zeta/render.hpp>

class App {
    private:
    bool m_running = true;
    Fps m_fps;

    public:
    void init();
    void run();
    void quit();
};