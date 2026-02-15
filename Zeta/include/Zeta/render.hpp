#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vector>

namespace Zeta {
    class Renderer {
    public:
    Renderer(vk::raii::Context& context, 
        vk::raii::Instance& instance, 
        struct wl_display* display, 
        struct wl_surface* surface, 
        uint32_t width, uint32_t height);
        void draw_frame();
        void recreate_swapchain(uint32_t width, uint32_t height);

    private:
        // Config
        const int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t m_current_frame = 0;
        vk::raii::SurfaceKHR m_surface = nullptr; 
        // Core RAII Handles
        vk::raii::Device m_device = nullptr;
        vk::raii::Queue m_graphics_queue = nullptr;
        vk::raii::SwapchainKHR m_swapchain = nullptr;
        
        // Sync Objects
        std::vector<vk::raii::Semaphore> m_image_available_semaphores;
        std::vector<vk::raii::Semaphore> m_render_finished_semaphores;
        std::vector<vk::raii::Fence> m_in_flight_fences;

        // Command Pool and Buffers
        vk::raii::CommandPool m_command_pool = nullptr;
        std::vector<vk::raii::CommandBuffer> m_command_buffers;

        vk::raii::SurfaceKHR* m_surface;

        // Internal Helpers
        uint32_t m_queue_family_index;
        void create_sync_objects();
        void create_command_resources();
    };
}