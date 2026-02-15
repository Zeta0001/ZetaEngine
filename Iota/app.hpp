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

    vk::raii::Context m_context;
    vk::raii::Instance m_instance;
    
    // 2. Window needs the instance to create the Wayland surface
    Zeta::Window m_window;
    
    // 3. Renderer depends on the Instance and the Window's surface
    Zeta::Renderer m_renderer;

    public:
    App();
    void init();
    void run();
    void quit();
};