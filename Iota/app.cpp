#include "app.hpp"



void App::init() {
    std::println("init app!");
};
void App::run() {

	Zeta::Window win(800, 600, "Zeta Engine Window");
	Zeta::Render renderer("Iota App", win);

    while (m_running == true) {
		m_fps.begin();
		

		win.pollEvents();

		renderer.drawFrame();

		if (win.shouldClose()) {
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