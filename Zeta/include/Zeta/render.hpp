#pragma once
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan_raii.hpp>
#include "Zeta/window.hpp"
#include <vector>
#include <string>

namespace Zeta {

class Render {
public:
    Render(const char* appName, const Window& window);
    ~Render() = default;

    Render(const Render&) = delete;
    Render& operator=(const Render&) = delete;

    void drawFrame();
    void recreate_swapchain(int width, int height);

private:
    // INITIALIZATION ORDER IS MANDATORY FOR RAII
    vk::raii::Context m_context;
    vk::raii::Instance m_instance;
    vk::raii::SurfaceKHR m_surface;
    vk::raii::PhysicalDevice m_physicalDevice;
    vk::raii::Device m_device;
    vk::raii::Queue m_graphicsQueue;
    vk::raii::SwapchainKHR m_swapchain = nullptr;
    vk::raii::CommandPool m_commandPool;

    // Synchronisation per frame in flight
    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_inFlightFences;

    // Swapchain and command resources

    std::vector<vk::raii::CommandBuffer> m_commandBuffers;

    std::vector<vk::Image> m_swapchainImages;

    // Frames in Flight configuration
    const int MAX_FRAMES_IN_FLIGHT = 2; // Standard for parallelism without high latency
    size_t m_currentFrame = 0;

    uint32_t m_graphicsQueueFamilyIndex; // Store this for pool creation

    // Helper methods called in initializer list
    vk::raii::Instance createInstance(const char* appName);
    vk::raii::PhysicalDevice pickPhysicalDevice();
    vk::raii::Device createLogicalDevice();
    vk::raii::SwapchainKHR setupSwapchain(uint32_t w, uint32_t h);
};

} // namespace Zeta