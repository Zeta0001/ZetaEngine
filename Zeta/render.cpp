#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "Zeta/render.hpp"
#include <iostream>

#include <vulkan/vulkan_raii.hpp>
#include "xdg-shell-client-protocol.h"
namespace Zeta {

Renderer::Renderer(wl_display* display, wl_surface* surface, uint32_t width, uint32_t height) : 
    m_context(),
    m_instance(create_instance()),
    m_surface(create_surface(display, surface)),
    m_physicalDevice(create_physical_device()),
    m_device(create_logical_device()),
    m_graphicsQueue(create_graphics_queue()),
    m_swapchain(create_swapchain(width, height)),
    m_commandPool(create_command_pool()),
    m_commandBuffers(create_command_buffers())
{

}

void Renderer::init() {
    // 4. Initial Setup
    m_swapchainImages = m_swapchain.getImages();
    create_sync_objects();
}

void Renderer::create_context() {
}

vk::raii::Instance Renderer::create_instance() {
    vk::ApplicationInfo appInfo{
        .pApplicationName = "Zeta App", // Quotes added
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Zeta Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3 // Required field
    };
    // 1. Define the required extensions for Wayland
    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,         // "VK_KHR_surface"
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME  // "VK_KHR_wayland_surface"
    };

    // 2. Set up the Instance creation info
    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    // 3. Return the RAII Instance
    // Note: m_context must be a vk::raii::Context
    return vk::raii::Instance(m_context, createInfo);
}

vk::raii::SurfaceKHR Renderer::create_surface(wl_display* display, wl_surface* surface) {
    vk::WaylandSurfaceCreateInfoKHR createInfo{
        .flags = {},        // Reserved for future use
        .display = display, // Your wl_display*
        .surface = surface  // Your wl_surface*
    };
    // 2. Use the RAII Instance (assumed member 'instance') to create the surface
    // The raii::SurfaceKHR constructor automatically manages the handle's lifetime
    return vk::raii::SurfaceKHR(m_instance, createInfo); 
}

vk::raii::PhysicalDevice Renderer::create_physical_device(){
    // 1. Enumerate all available physical devices
    // Returns a vk::raii::PhysicalDevices (which acts like a std::vector<vk::raii::PhysicalDevice>)
    vk::raii::PhysicalDevices physicalDevices(m_instance);

    if (physicalDevices.empty()) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    // 2. Select a suitable device
    // You can loop through them to check properties/features
    for (const auto& device : physicalDevices) {
        auto properties = device.getProperties();
        
        // Example: Prefer a discrete GPU
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            return device; 
        }
    }

    // Default to the first available device if no discrete GPU is found
    return physicalDevices[0];
};

uint32_t Renderer::find_queue_family() {
    // 1. Get queue family properties from the physical device
    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        // 2. Check for Graphics support
        bool supportsGraphics = !!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics);

        // 3. Check for Surface/Presentation support (Wayland)
        // m_surface is the vk::raii::SurfaceKHR you created earlier
        bool supportsPresent = m_physicalDevice.getSurfaceSupportKHR(i, *m_surface);

        if (supportsGraphics && supportsPresent) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find a suitable queue family!");
}

vk::raii::Device Renderer::create_logical_device(){
    // 1. Specify the queue(s) to be created. 
    // Usually, you'd use a member 'queueFamilyIndex' found during PhysicalDevice selection.
    m_queueFamilyIndex = find_queue_family();

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = m_queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    // 2. Define required device-level extensions
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME // Required for presenting images to a surface
    };

    // 3. Set up the logical device creation info
    vk::DeviceCreateInfo createInfo{
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    // 4. Instantiate the RAII Device
    // This requires the physical device and the create info.
    return vk::raii::Device(m_physicalDevice, createInfo);
};

vk::raii::Queue Renderer::create_graphics_queue() {
    // getQueue(uint32_t queueFamilyIndex, uint32_t queueIndex)
    // Returns a vk::raii::Queue object
    return vk::raii::Queue(m_device, m_queueFamilyIndex, 0);
}

vk::raii::SwapchainKHR Renderer::create_swapchain(uint32_t width, uint32_t height) {
    // 1. Query basic surface capabilities
    auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
    auto formats = m_physicalDevice.getSurfaceFormatsKHR(*m_surface);
    
    // 2. Select format (usually B8G8R8A8_SRGB is a safe bet)
    vk::SurfaceFormatKHR surfaceFormat = formats[0]; 
    for (const auto& f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            surfaceFormat = f;
            break;
        }
    }

    // 3. Configure the swapchain extent (clamped to surface limits)
    vk::Extent2D extent{
        std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };

    // 4. Set image count (usually triple buffering)
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // 5. Build the Swapchain info
    vk::SwapchainCreateInfoKHR createInfo{
        .surface = *m_surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive, // Assuming graphics and present queue are the same
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo, // Guaranteed to be supported
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr
    };

    return vk::raii::SwapchainKHR(m_device, createInfo);
}

void Renderer::create_sync_objects() {
    // 1. Prepare creation info for Binary Semaphores
    vk::SemaphoreCreateInfo binaryInfo{};

    // 2. Prepare creation info for Timeline Semaphore
    vk::SemaphoreTypeCreateInfo timelineTypeInfo{
        .semaphoreType = vk::SemaphoreType::eTimeline,
        .initialValue = 0
    };
    vk::SemaphoreCreateInfo timelineInfo{
        .pNext = &timelineTypeInfo
    };

    // 3. Populate vectors with RAII objects
    m_imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_imageAvailableSemaphores.emplace_back(m_device, binaryInfo);
        m_renderFinishedSemaphores.emplace_back(m_device, binaryInfo);
    }

    // 4. Create the single timeline semaphore
    m_frameTimeline = vk::raii::Semaphore(m_device, timelineInfo);
}


vk::raii::CommandPool Renderer::create_command_pool() {
    // 1. Create the Command Pool
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_queueFamilyIndex
    };
    return vk::raii::CommandPool(m_device, poolInfo);
}

vk::raii::CommandBuffers Renderer::create_command_buffers() {
    // 2. Allocate Command Buffers
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = *m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
    };

    // This returns a vk::raii::CommandBuffers object
    // which contains the vector of individual command buffers
    return vk::raii::CommandBuffers(m_device, allocInfo);
}

void Renderer::draw_frame() {
    // 1. CPU-SIDE SYNCHRONIZATION
    // Wait for the GPU to finish the work of (Current - MAX_FRAMES_IN_FLIGHT)
    if (m_currentFrameCounter >= MAX_FRAMES_IN_FLIGHT) {
        uint64_t waitValue = m_currentFrameCounter - MAX_FRAMES_IN_FLIGHT + 1;
        vk::SemaphoreWaitInfo waitInfo{
            .semaphoreCount = 1,
            .pSemaphores = &(*m_frameTimeline),
            .pValues = &waitValue
        };
        // Wait indefinitely for the GPU to catch up
        auto waitResult = m_device.waitSemaphores(waitInfo, UINT64_MAX);
    }

    // 2. ACQUIRE IMAGE FROM SWAPCHAIN
    uint32_t syncIndex = m_currentFrameCounter % MAX_FRAMES_IN_FLIGHT;
    
    // acquireNextImage returns a vk::ResultValue tuple
    auto [result, imageIndex] = m_swapchain.acquireNextImage(
        UINT64_MAX, 
        *m_imageAvailableSemaphores[syncIndex]
    );

    // 3. COMMAND RECORDING
    auto& cmd = m_commandBuffers[syncIndex];
    vk::Image image = m_swapchainImages[imageIndex];

    cmd.reset();
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    // Transition: Undefined -> Transfer Destination
    vk::ImageMemoryBarrier2 barrier_to_clear{
        .dstStageMask = vk::PipelineStageFlagBits2::eClear,
        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .image = image,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{ .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier_to_clear });

    // CLEAR TO ORANGE
    vk::ClearColorValue orange_color(std::array<float, 4>{1.0f, 0.5f, 0.0f, 1.0f});
    vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    cmd.clearColorImage(image, vk::ImageLayout::eTransferDstOptimal, orange_color, range);

    // Transition: Transfer Destination -> Present
    vk::ImageMemoryBarrier2 barrier_to_present{
        .srcStageMask = vk::PipelineStageFlagBits2::eClear,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands, // Required for present safety
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .image = image,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{ .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier_to_present });

    cmd.end();

    // 4. SUBMIT TO QUEUE
    m_currentFrameCounter++; // Increment: this frame is now assigned this new value

    vk::SemaphoreSubmitInfo waitSemaphoreInfo{
        .semaphore = *m_imageAvailableSemaphores[syncIndex],
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };

    // Signal two things: The binary semaphore for present, and the timeline for CPU sync
    std::array<vk::SemaphoreSubmitInfo, 2> signalSemaphoreInfos = {{
        {
            .semaphore = *m_renderFinishedSemaphores[syncIndex],
            .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
        },
        {
            .semaphore = *m_frameTimeline,
            .value = m_currentFrameCounter, 
            .stageMask = vk::PipelineStageFlagBits2::eAllCommands
        }
    }};

    vk::CommandBufferSubmitInfo cmdBufferInfo{ .commandBuffer = *cmd };

    vk::SubmitInfo2 submitInfo{
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdBufferInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfos.size()),
        .pSignalSemaphoreInfos = signalSemaphoreInfos.data()
    };

    m_graphicsQueue.submit2(submitInfo);

    // 5. PRESENTATION
    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &(*m_renderFinishedSemaphores[syncIndex]),
        .swapchainCount = 1,
        .pSwapchains = &(*m_swapchain),
        .pImageIndices = &imageIndex
    };

    // presentKHR returns a vk::Result
    auto presentResult = m_graphicsQueue.presentKHR(presentInfo);
}


void Renderer::recreate_swapchain(uint32_t width, uint32_t height) {
}


} // namespace Zeta