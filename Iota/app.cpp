#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#include "app.hpp"
#include <vulkan/vulkan_raii.hpp>


App::App() : m_window(800, 600), m_renderer(){};

void App::init() {
    std::println("init app!");
	m_window.set_resize_callback([this](uint32_t w, uint32_t h) {
        this->m_eventBus.push(Zeta::ResizeEvent{w, h});
    });
	m_window.set_key_callback([this](uint32_t key, bool pressed) {
		this->m_eventBus.push(Zeta::KeyEvent{key, pressed});
	});
	m_renderer.init(m_window.get_display(), m_window.get_surface(), m_window.m_width, m_window.m_height);
};
void App::run() {

    while (m_running == true) {


		m_fps.begin();
		//std::this_thread::sleep_for(std::chrono::milliseconds(16));
		m_window.poll_events();
		Zeta::Event e;
        while (m_eventBus.poll(e)) {
            std::visit([this](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Zeta::QuitEvent>) {
					
                    m_running = false;
                } else if constexpr (std::is_same_v<T, Zeta::ResizeEvent>) {
                    m_renderer.handle_resize(arg.width, arg.height);
					//m_window.acknowledge_resize();
                }
				else if constexpr (std::is_same_v<T, Zeta::KeyEvent>) {
					m_window.update(arg);
					if (arg.key == 1 && arg.pressed) { m_running = false; } 
				}
            }, e);
        }
        // 2. Handle Resizing Handshake
		// if (m_window.m_resize_pending) {
		// 	m_window.acknowledge_resize(); // Replaces direct access to private members
		// 	m_renderer.recreate_swapchain(m_window.m_width, m_window.m_height);
		// 	m_window.m_resize_pending = false;
		// }

        // 3. Render
        m_renderer.draw_frame();
		m_fps.end();
		static int counter = 0;
		if (counter++ > 200) {
			counter = 0;
			std::println("fps1: {}", m_fps.getFps());
			#ifndef NDEBUG
			std::println("DEBUG");
			#endif
		}
	}

};
void App::quit() {
};