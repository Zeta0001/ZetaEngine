#include "app.hpp"



void App::init() {
    std::println("init app!");
};
void App::run() {

	Zeta::Window win(800, 600, "Zeta Engine Window");
	Zeta::Render renderer("Iota App", win);

    while (m_running == true) {
		m_fps.begin();
		

		win.poll_events();

		if (win.m_resize_pending) {
			// Acknowledge the Wayland event
			win.config();
			
			// RECREATE HERE: Match the Vulkan swapchain to the new Wayland size
			renderer.recreate_swapchain(win.m_width, win.m_height);
			
			win.m_resize_pending = false;
			continue; // Skip this frame to ensure everything is synced
		}

		try {
			renderer.drawFrame();
		} catch (const vk::OutOfDateKHRError& e) {
			// 3. Fallback: Catch Vulkan's internal resize detection
			win.m_resize_pending = true; 
		}

		if (win.should_close()) {
			m_running = false;
		}
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