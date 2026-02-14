#pragma once
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "Zeta/window.hpp" // Need Window for handles
#include <vector>

namespace Zeta {

class Render {
public:
Render(const char* appName, const Window& window);
    // Destructor is now empty/default because RAII handles cleanup!
    ~Render() = default;

    // Safety: RAII handles cannot be copied
    Render(const Render&) = delete;
    Render& operator=(const Render&) = delete;
    
    void drawFrame();
private:
    vk::raii::Context m_context;
    vk::raii::Instance m_instance;
    vk::raii::SurfaceKHR m_surface; // The bridge
    vk::raii::PhysicalDevice m_physicalDevice;
    vk::raii::Device m_device;
    vk::raii::Queue m_graphicsQueue;
    vk::raii::SwapchainKHR m_swapchain;
    vk::raii::Semaphore m_imageAvailableSemaphore;
    vk::raii::Semaphore m_renderFinishedSemaphore;
    vk::raii::CommandPool m_commandPool;
    vk::raii::CommandBuffer m_commandBuffer;
    std::vector<vk::Image> m_swapchainImages;

    vk::raii::Instance createInstance(const char* appName);
    vk::raii::PhysicalDevice pickPhysicalDevice();
    vk::raii::Device createLogicalDevice();
    vk::raii::SwapchainKHR setupSwapchain(uint32_t w, uint32_t h) ;

};

} // namespace Zeta