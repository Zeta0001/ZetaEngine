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

    vk::raii::Context m_context;     // 1. Context MUST be first
    vk::raii::Instance m_instance;   // 2. Instance
    Zeta::Window m_window;           // 3. Window (contains the Surface)
    Zeta::Renderer m_renderer;  

    public:
    App();
    void init();
    void run();
    void quit();
};