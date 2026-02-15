#include "Zeta/render.hpp"
#include <iostream>
#include <stdexcept>

namespace Zeta {

Render::Render(const char* appName, const Window& window) 
    : m_context(),
      m_instance(createInstance(appName)),
      m_surface(m_instance, vk::WaylandSurfaceCreateInfoKHR({}, window.get_display(), window.get_surface())),
      m_physicalDevice(pickPhysicalDevice()),
      m_device(createLogicalDevice()),
      m_graphicsQueue(m_device.getQueue(0, 0)), // Use family 0 for simplicity
      m_swapchain(setupSwapchain(2560, 1600)),
      m_commandPool(m_device, vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0))
      //m_commandBuffer(std::move(vk::raii::CommandBuffers(m_device, {*m_commandPool, vk::CommandBufferLevel::ePrimary, 1}).front())),
{
    m_swapchainImages = m_swapchain.getImages();
    // Initialize Sync Objects for Frames in Flight
    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled }; // Start signaled

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_imageAvailableSemaphores.emplace_back(m_device, semaphoreInfo);
        m_renderFinishedSemaphores.emplace_back(m_device, semaphoreInfo);
        m_inFlightFences.emplace_back(m_device, fenceInfo);
    }
    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            m_graphicsQueueFamilyIndex = i;
            break;
        }
    }
    
    // 2. Create the Command Pool using that index
    vk::CommandPoolCreateInfo poolInfo{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // Allow re-recording
        m_graphicsQueueFamilyIndex
    };
    m_commandPool = vk::raii::CommandPool(m_device, poolInfo);
    
    // 3. Allocate the Buffers (One per frame in flight)
    vk::CommandBufferAllocateInfo allocInfo{
        *m_commandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
    };
    
    // Use the assignment to populate the vector
    m_commandBuffers = vk::raii::CommandBuffers(m_device, allocInfo);
    std::cout << "Vulkan RAII System Ready. Surface Linked to Wayland." << std::endl;
    std::cout << "Using GPU: " << m_physicalDevice.getProperties().deviceName << std::endl;
}

vk::raii::Instance Render::createInstance(const char* appName) {
    vk::ApplicationInfo appInfo{appName, 1, "Zeta", 1, VK_API_VERSION_1_3};
    
    std::vector<const char*> extensions = { 
        VK_KHR_SURFACE_EXTENSION_NAME, 
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME 
    };

    return vk::raii::Instance(m_context, vk::InstanceCreateInfo({}, &appInfo, 0, nullptr, (uint32_t)extensions.size(), extensions.data()));
}

vk::raii::PhysicalDevice Render::pickPhysicalDevice() {
        // Inside your Renderer or Device selection logic
    auto devices = m_instance.enumeratePhysicalDevices();
    vk::raii::PhysicalDevice* selectedDevice = nullptr;

    for (auto& device : devices) {
        auto props = device.getProperties();
        if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            selectedDevice = &device;
            break; // Found the high-performance GPU
        }
    }

    // Fallback if no dGPU is found
    if (!selectedDevice) {
        selectedDevice = &devices[0]; 
    }

    std::cout << "Engine using GPU: " << selectedDevice->getProperties().deviceName << std::endl;

    //vk::raii::PhysicalDevices devices(m_instance);
    if (selectedDevice == nullptr) throw std::runtime_error("No Vulkan GPUs found");
    return *selectedDevice; 
}

vk::raii::Device Render::createLogicalDevice() {
    float priority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo({}, 0, 1, &priority);
    
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    
    vk::DeviceCreateInfo deviceInfo({}, 1, &queueInfo, 0, nullptr, (uint32_t)deviceExtensions.size(), deviceExtensions.data());
    return vk::raii::Device(m_physicalDevice, deviceInfo);
}

vk::raii::SwapchainKHR Render::setupSwapchain(uint32_t w, uint32_t h) {
    vk::SwapchainCreateInfoKHR info{};
    info.surface = *m_surface;
    info.minImageCount = 3;
    info.imageFormat = vk::Format::eB8G8R8A8Unorm;
    info.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    info.imageExtent = vk::Extent2D{w, h};
    info.imageArrayLayers = 1;
    info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    info.presentMode = vk::PresentModeKHR::eImmediate;

    return vk::raii::SwapchainKHR(m_device, info);
}

void Render::recreate_swapchain(int width, int height) {
    // 1. Wait for GPU to finish work
    m_device.waitIdle();

    // 2. Destroy dependent RAII objects (Framebuffers and ImageViews)
    // Assuming these are std::vector<vk::raii::Framebuffer> etc.
    //m_swapchain_framebuffers.clear();
    //m_swapchain_image_views.clear();

    // 3. Configure new swapchain
    vk::Extent2D extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = *m_surface; // Dereference raii::SurfaceKHR to get raw handle
    createInfo.minImageCount = 3;
    createInfo.imageFormat = vk::Format::eB8G8R8A8Unorm;
    createInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = vk::PresentModeKHR::eImmediate;
    // Smoothness: Use Mailbox for 165Hz jitter
    
    // Pass raw handle of current RAII swapchain for smooth transition
    createInfo.oldSwapchain = *m_swapchain; 

    // 4. Create new RAII swapchain
    // Move-assignment here destroys the old m_swapchain AFTER the new one is ready
    m_swapchain = vk::raii::SwapchainKHR(m_device, createInfo);

    std::cout << "resize swapchaaiaiaina" << std::endl;
    // 5. Re-initialize your dependent resources
    //create_image_views();
    //create_framebuffers();
}

void Render::drawFrame() {
    (void)m_device.waitForFences({*m_inFlightFences[m_currentFrame]}, true, UINT64_MAX);

    // Acquire swapchain image
    auto [result, imageIndex] = m_swapchain.acquireNextImage(UINT64_MAX, *m_imageAvailableSemaphores[m_currentFrame]);

    m_device.resetFences({*m_inFlightFences[m_currentFrame]});

    // Reset and begin re-recording the buffer for this frame slot
    auto& cmd = m_commandBuffers[m_currentFrame];
    cmd.reset(); 
    
    vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    cmd.begin(beginInfo);

    // ... your rendering commands (Viewport, Scissor, Draw) ...

    cmd.end();

    // 4. Submit
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::SubmitInfo submitInfo{};
    submitInfo.setWaitSemaphores(*m_imageAvailableSemaphores[m_currentFrame]);
    submitInfo.setWaitDstStageMask(waitStages);
    submitInfo.setCommandBuffers(*cmd); // Submit the buffer indexed by currentFrame
    submitInfo.setSignalSemaphores(*m_renderFinishedSemaphores[m_currentFrame]);

    m_graphicsQueue.submit(submitInfo, *m_inFlightFences[m_currentFrame]);

    // 5. Present and Advance
    vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphores(*m_renderFinishedSemaphores[m_currentFrame]);
    presentInfo.setSwapchains(*m_swapchain);
    presentInfo.setImageIndices(imageIndex);

    (void)m_graphicsQueue.presentKHR(presentInfo);
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

} // namespace Zeta