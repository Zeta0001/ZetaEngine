#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#include "app.hpp"
#include <vulkan/vulkan_raii.hpp>

App::App() : m_window(2560, 1600, "Zeta Engine"), m_renderer(m_window.get_display(), m_window.get_surface(), m_window.m_width, m_window.m_height){};

void App::init() {
    std::println("init app!");
    m_window.init();
	m_renderer.init();
};
void App::run() {

    while (m_running == true) {
		m_fps.begin();
		//std::this_thread::sleep_for(std::chrono::milliseconds(16));
		m_window.poll_events();

        // 2. Handle Resizing Handshake
		if (m_window.m_resize_pending) {
			m_window.acknowledge_resize(); // Replaces direct access to private members
			m_renderer.recreate_swapchain(m_window.m_width, m_window.m_height);
			m_window.m_resize_pending = false;
		}

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