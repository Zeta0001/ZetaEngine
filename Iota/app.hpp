#pragma once
#include <print>
#include <vector>

#include <Zeta/time.hpp>
#include <Zeta/window.hpp>
#include <Zeta/render.hpp>
#include <Zeta/events.hpp>

class App {
    private:
    bool m_running = true;
    Fps m_fps;

    struct SpawnEnemyEvent { float x, y; };
    struct ToggleMenuEvent {};

    // Compose the variants: Iota events + Zeta's Core events
    using AppEvent = std::variant<
        Zeta::QuitEvent, 
        Zeta::ResizeEvent, 
        Zeta::KeyEvent, 
        SpawnEnemyEvent, 
        ToggleMenuEvent
    >;

    Zeta::Window m_window;           // 3. Window (contains the Surface)
    Zeta::Renderer m_renderer;  
    Zeta::EventBus<AppEvent> m_eventBus;
    
    public:
    App();
    void init();
    void run();
    void quit();
};