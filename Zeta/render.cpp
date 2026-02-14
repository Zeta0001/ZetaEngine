#include "Zeta/render.hpp"
#include <iostream>

namespace Zeta {

    Render::Render(const char* appName, const Window& window) 
    : m_instance(createInstance(appName)),
      m_surface(m_instance, vk::WaylandSurfaceCreateInfoKHR({}, window.get_display(), window.get_surface())),
      m_physicalDevice(pickPhysicalDevice()),
      m_device(createLogicalDevice()),
      m_graphicsQueue(m_device.getQueue(0, 0)),
      m_swapchain(setupSwapchain(800, 600)),
      // 1. Create the Command Pool
      m_commandPool(m_device, vk::CommandPoolCreateInfo(
          vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0)),
      // 2. Allocate the Command Buffer
      m_commandBuffer(std::move(vk::raii::CommandBuffers(m_device, 
          vk::CommandBufferAllocateInfo(*m_commandPool, vk::CommandBufferLevel::ePrimary, 1)).front())),
      m_imageAvailableSemaphore(m_device, vk::SemaphoreCreateInfo()),
      m_renderFinishedSemaphore(m_device, vk::SemaphoreCreateInfo())
{
    // 3. Get the images from the swapchain so we can clear them
    m_swapchainImages = m_swapchain.getImages();
}

vk::raii::Instance Render::createInstance(const char* appName) {
    vk::ApplicationInfo appInfo{appName, 1, "Zeta", 1, VK_API_VERSION_1_3};

    // 2. You MUST include these two extensions
    std::vector<const char*> extensions = { 
        VK_KHR_SURFACE_EXTENSION_NAME, 
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME 
    };
    
    vk::InstanceCreateInfo createInfo({}, &appInfo, 0, nullptr, 
                                     (uint32_t)extensions.size(), extensions.data());

    return vk::raii::Instance(m_context, createInfo);
}


vk::raii::PhysicalDevice Render::pickPhysicalDevice() {
    vk::raii::PhysicalDevices devices(m_instance);
    if (devices.empty()) throw std::runtime_error("No Vulkan GPUs found");
    

    auto extensionProperties = m_physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsSwapchain = false;
    for (const auto& ext : extensionProperties) {
        if (std::string(ext.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
            supportsSwapchain = true;
            break;
        }
    }
    if (!supportsSwapchain) throw std::runtime_error("GPU does not support swapchains!");

    // Return the first available GPU
    return devices[0];
}

vk::raii::Device Render::createLogicalDevice() {
    float priority = 1.0f;
    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();
    uint32_t graphicsFamily = 0;
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsFamily = i;
            break;
        }
    }

vk::DeviceQueueCreateInfo queueInfo({}, graphicsFamily, 1, &priority);

    // 1. Define the required device extensions
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    vk::DeviceCreateInfo deviceInfo(
        {}, 
        1, &queueInfo,                   // Queues
        0, nullptr,                      // Layers (deprecated)
        static_cast<uint32_t>(deviceExtensions.size()), 
        deviceExtensions.data()          // <--- THIS WAS MISSING
    );

    return vk::raii::Device(m_physicalDevice, deviceInfo);
}

vk::raii::SwapchainKHR Render::setupSwapchain(uint32_t w, uint32_t h) {
    vk::SwapchainCreateInfoKHR info{};
    info.surface = *m_surface;
    info.minImageCount = 3; // Triple buffering
    info.imageFormat = vk::Format::eB8G8R8A8Unorm;
    info.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    info.imageExtent = vk::Extent2D{w, h};
    info.imageArrayLayers = 1;
    info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    info.imageSharingMode = vk::SharingMode::eExclusive;
    info.preTransform = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface).currentTransform;
    info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    info.presentMode = vk::PresentModeKHR::eFifo;
    info.clipped = VK_TRUE;

    return vk::raii::SwapchainKHR(m_device, info);
}

void Render::drawFrame() {
    auto [result, imageIndex] = m_swapchain.acquireNextImage(UINT64_MAX, *m_imageAvailableSemaphore);

    // Record Commands
    m_commandBuffer.reset();
    m_commandBuffer.begin(vk::CommandBufferBeginInfo{});

    // NOTE: Image must be in eGeneral or eTransferDstOptimal layout to use clearColorImage
    // For a "correct" engine, you'd use a PipelineBarrier here to transition the layout.
    
    vk::ClearColorValue clearColor(std::array<float, 4>{0.1f, 0.1f, 0.2f, 1.0f});
    vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    
    m_commandBuffer.clearColorImage(m_swapchainImages[imageIndex], vk::ImageLayout::eGeneral, clearColor, range);

    m_commandBuffer.end();

    // Submit to Queue
    vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo(*m_imageAvailableSemaphore, waitStages, *m_commandBuffer, *m_renderFinishedSemaphore);
    
    m_graphicsQueue.submit(submitInfo);

    // Present
    vk::PresentInfoKHR presentInfo(*m_renderFinishedSemaphore, *m_swapchain, imageIndex);
    (void)m_graphicsQueue.presentKHR(presentInfo);
    
    // Wait for GPU to finish so we don't overwrite the command buffer while it's running
    m_graphicsQueue.waitIdle();
}
} // namespace Zeta