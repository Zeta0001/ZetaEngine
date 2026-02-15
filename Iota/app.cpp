#define VK_USE_PLATFORM_WAYLAND_KHR
#include "app.hpp"
#include <vulkan/vulkan_raii.hpp>

App::App() : 
    m_context(),
    // Initialize Instance with Validation Layers if in Debug
    m_instance(m_context, []() {
        vk::InstanceCreateInfo info;
        std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> extensions = { 
			vk::KHRSurfaceExtensionName, 
			vk::KHRWaylandSurfaceExtensionName // Use the C++ constant
		};
        info.setPEnabledLayerNames(layers);
        info.setPEnabledExtensionNames(extensions);
        return info;
    }()),
    // Create window (which creates the wl_surface and vk::raii::SurfaceKHR)
	m_window(800, 600, "Zeta Engine"), // Matches Window(int, int, string)
    m_renderer(m_context, m_window.get_surface(), 800, 600) // Must return vk::raii::SurfaceKHR
{}

void App::init() {
    std::println("init app!");
};
void App::run() {

    while (m_running == true) {
		m_fps.begin();
		
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
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