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
        //const int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t m_current_frame = 0;

        vk::raii::Context m_context;
        vk::raii::Instance m_instance;
        vk::raii::SurfaceKHR m_surface;
        vk::raii::PhysicalDevice m_physicalDevice;
        vk::raii::Device m_device;
        vk::raii::Queue m_graphicsQueue;
        vk::raii::SwapchainKHR m_swapchain;
        vk::raii::CommandPool m_commandPool;
        vk::raii::CommandBuffers m_commandBuffers; // RAII vectors handle their own lifetime
        // Sync Objects
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        // Binary semaphores (one per frame-in-flight slot)
        std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
        std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
        
        // Single Timeline semaphore
        vk::raii::Semaphore m_frameTimeline{nullptr};
        uint64_t m_currentFrameCounter = 0;

        std::vector<vk::Image> m_swapchainImages;
        vk::Extent2D m_swapchainExtent;
        vk::Format m_swapchainFormat;
        std::vector<vk::raii::ImageView> m_swapchainImageViews;

        vk::raii::DebugUtilsMessengerEXT m_debugMessenger{nullptr};
        // Command Pool and Buffers


        wl_display* m_display;


        std::optional<vk::raii::QueryPool> m_query_pool;
        std::vector<uint64_t> m_query_results;
        float m_timestamp_period; // Period in nanoseconds per tick
    
        void create_query_pool();


        // Initialization Functions
        void create_context();
        vk::raii::Instance create_instance();
        vk::raii::SurfaceKHR create_surface(wl_display* display, wl_surface* surface);
        vk::raii::PhysicalDevice create_physical_device();
        uint32_t find_queue_family();
        vk::raii::Device create_logical_device();
        vk::raii::Queue create_graphics_queue();
        vk::raii::SwapchainKHR create_swapchain(uint32_t width, uint32_t height);
        vk::raii::CommandPool create_command_pool();
        vk::raii::CommandBuffers create_command_buffers();

        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        uint32_t m_queueFamilyIndex = 0;

        void create_sync_objects();

        vk::raii::PipelineLayout m_pipelineLayout{nullptr};
        vk::raii::Pipeline m_graphicsPipeline{nullptr};
        void create_graphics_pipeline();
    };
}