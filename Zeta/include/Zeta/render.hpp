#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include <vector>

namespace Zeta {
    class Renderer {
    public:
    Renderer();
        void init(wl_display* display, wl_surface* surface, uint32_t width, uint32_t height);
        void draw_frame();
        void recreate_swapchain(uint32_t width, uint32_t height);

    private:
        // Config
        const int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t m_current_frame = 0;

        std::optional<vk::raii::Context> m_context;
        std::optional<vk::raii::Instance> m_instance;
        std::optional<vk::raii::SurfaceKHR> m_surface;
        std::optional<vk::raii::PhysicalDevice> m_physical_device;
        std::optional<vk::raii::Device> m_device;
        std::optional<vk::raii::Queue> m_graphics_queue;
        std::optional<vk::raii::SwapchainKHR> m_swapchain;
        
        // Sync Objects
        std::vector<vk::raii::Semaphore> m_image_available_sems;
        std::vector<vk::raii::Semaphore> m_render_finished_sems;
        std::vector<vk::raii::Fence> m_in_flight_fences;
        std::vector<vk::Image> m_swapchain_images; // Raw handles retrieved from RAII swapchain
        std::vector<vk::Fence> m_images_in_flight;

        std::optional<vk::raii::Buffer> m_dummy_buffer;
std::optional<vk::raii::DeviceMemory> m_dummy_memory;

        // Command Pool and Buffers
        std::optional<vk::raii::CommandPool> m_command_pool;
        std::vector<vk::raii::CommandBuffer> m_command_buffers; // RAII vectors handle their own lifetime

        struct wl_display* m_display;


        std::optional<vk::raii::QueryPool> m_query_pool;
        std::vector<uint64_t> m_query_results;
        float m_timestamp_period; // Period in nanoseconds per tick
    
        void create_query_pool();


        // Initialization Functions
        void create_context();
        void create_instance();
        void create_surface(struct wl_display* display, struct wl_surface* surface);
        void create_device();
        void create_graphics_queue();
        void create_swapchain(uint32_t width, uint32_t height);
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        uint32_t m_queue_family_index = 0;



// Add to your private methods
        void create_command_resources();
        void create_sync_objects();
    };
}