#pragma once
//#define VK_USE_PLATFORM_WAYLAND_KHR
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

private:
    // INITIALIZATION ORDER IS MANDATORY FOR RAII
    vk::raii::Context m_context;
    vk::raii::Instance m_instance;
    vk::raii::SurfaceKHR m_surface;
    vk::raii::PhysicalDevice m_physicalDevice;
    vk::raii::Device m_device;
    vk::raii::Queue m_graphicsQueue;
    vk::raii::SwapchainKHR m_swapchain;
    
    vk::raii::CommandPool m_commandPool;
    vk::raii::CommandBuffer m_commandBuffer;
    
    vk::raii::Semaphore m_imageAvailableSemaphore;
    vk::raii::Semaphore m_renderFinishedSemaphore;

    std::vector<vk::Image> m_swapchainImages;

    // Helper methods called in initializer list
    vk::raii::Instance createInstance(const char* appName);
    vk::raii::PhysicalDevice pickPhysicalDevice();
    vk::raii::Device createLogicalDevice();
    vk::raii::SwapchainKHR setupSwapchain(uint32_t w, uint32_t h);
};

} // namespace Zeta