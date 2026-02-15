#define VK_USE_PLATFORM_WAYLAND_KHR
#include "Zeta/render.hpp"
#include <iostream>

#include <vulkan/vulkan_raii.hpp>
namespace Zeta {

    Renderer::Renderer() {

}

void Renderer::init(wl_display* display, wl_surface* surface, uint32_t width, uint32_t height) {
    create_context();
    create_instance();
    create_surface(display, surface);
    create_device();
    create_graphics_queue();
    create_swapchain(width, height);

    // 4. Initial Setup
    create_sync_objects();
    create_command_resources();
    //recreate_swapchain(width, height);

}

void Renderer::create_context() {
    m_context.emplace(); // Default constructor for Context
}

void Renderer::create_instance() {
    static const std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
    static const std::vector<const char*> extensions = { 
        vk::KHRSurfaceExtensionName, 
        vk::KHRWaylandSurfaceExtensionName 
    };
    vk::ApplicationInfo appInfo("Zeta", 1, "Zeta", 1, vk::ApiVersion13);
    
    vk::InstanceCreateInfo info({}, &appInfo, layers, extensions);
    
    // Use emplace to call the Instance constructor: (context, info)
    m_instance.emplace(*m_context, info);
}

void Renderer::create_surface(struct wl_display* display, struct wl_surface* surface) {
    vk::WaylandSurfaceCreateInfoKHR info({}, display, surface);
    m_surface.emplace(*m_instance, info);
}

void Renderer::create_device() {
    auto phys_devices = m_instance->enumeratePhysicalDevices();
    auto& phys_device = phys_devices.front(); // Simplified; use scoring here

    // Find queue family
    auto families = phys_device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            m_queue_family_index = i;
            break;
        }
    }

    float priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_info({}, m_queue_family_index, 1, &priority);
    std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    
    vk::DeviceCreateInfo device_info({}, queue_info, {}, extensions);
    m_device.emplace(phys_device, device_info);
}

void Renderer::create_graphics_queue() {
    m_graphics_queue.emplace(*m_device, m_queue_family_index, 0);
}

void Renderer::create_swapchain(uint32_t width, uint32_t height) {
    vk::SwapchainCreateInfoKHR info;
    info.setSurface(**m_surface); // Double dereference for optional -> handle
    info.setMinImageCount(3);
    info.setImageFormat(vk::Format::eB8G8R8A8Unorm);
    info.setImageExtent({width, height});
    info.setImageArrayLayers(1);
    info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    info.setPresentMode(vk::PresentModeKHR::eMailbox);
    info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

    m_swapchain.emplace(*m_device, info);
}

void Renderer::create_sync_objects() {
    vk::SemaphoreCreateInfo sem_info;
    vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Use *m_device to pass the vk::raii::Device reference
        m_image_available_semaphores.emplace_back(*m_device, sem_info);
        m_render_finished_semaphores.emplace_back(*m_device, sem_info);
        m_in_flight_fences.emplace_back(*m_device, fence_info);
    }
}


void Renderer::create_command_resources() {
    vk::CommandPoolCreateInfo pool_info(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 
        m_queue_family_index
    );
    m_command_pool.emplace(*m_device, pool_info);

    vk::CommandBufferAllocateInfo alloc_info(
        **m_command_pool, 
        vk::CommandBufferLevel::ePrimary, 
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
    );
    // RAII CommandBuffers returns a vector, so we can assign directly
    m_command_buffers = m_device->allocateCommandBuffers(alloc_info);
}

void Renderer::draw_frame() {
    // 1. Throttle: Wait for the specific frame's fence
    if (m_device->waitForFences({*m_in_flight_fences[m_current_frame]}, true, UINT64_MAX) != vk::Result::eSuccess) return;

    // 2. Acquire
    uint32_t image_index;
    try {
        // acquireNextImage returns a ResultValue containing (Result, uint32_t)
        auto result = m_swapchain->acquireNextImage(UINT64_MAX, *m_image_available_semaphores[m_current_frame]);
        image_index = result.value;
    } catch (const vk::OutOfDateKHRError&) {
        // Signal app to recreate swapchain
        return; 
    }

    m_device->resetFences({*m_in_flight_fences[m_current_frame]});

    // 3. Record
    auto& cmd = m_command_buffers[m_current_frame];
    cmd.reset();
    cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    
    // ... Clear/Draw calls ...
    
    cmd.end();

    // 4. Submit
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::SubmitInfo submit_info(
        *m_image_available_semaphores[m_current_frame], 
        wait_stages, 
        *cmd, 
        *m_render_finished_semaphores[m_current_frame]
    );
    
    m_graphics_queue->submit(submit_info, *m_in_flight_fences[m_current_frame]);

    // 5. Present
    vk::PresentInfoKHR present_info(
        *m_render_finished_semaphores[m_current_frame], 
        **m_swapchain, 
        image_index
    );
    
    try {
        (void)m_graphics_queue->presentKHR(present_info);
    } catch (const vk::OutOfDateKHRError&) {
        // Handle resize
    }

    m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
void Renderer::recreate_swapchain(uint32_t width, uint32_t height) {
    m_device->waitIdle();

    vk::SwapchainCreateInfoKHR info;
    info.setSurface(**m_surface);
    info.setMinImageCount(3); // Triple buffering for 1600p
    info.setImageFormat(vk::Format::eB8G8R8A8Unorm);
    info.setImageExtent({width, height});
    info.setImageArrayLayers(1);
    info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    info.setPresentMode(vk::PresentModeKHR::eMailbox);
    info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

    // Pass the old handle if it exists
    if (m_swapchain.has_value()) {
        info.setOldSwapchain(**m_swapchain);
    }

    // emplace destroys the old swapchain and moves the new one in
    m_swapchain.emplace(*m_device, info);
}

} // namespace Zeta