#define VK_USE_PLATFORM_WAYLAND_KHR
#include "Zeta/render.hpp"
#include <iostream>

#include <vulkan/vulkan_raii.hpp>
namespace Zeta {

    Renderer::Renderer(vk::raii::Context& context, 
        vk::raii::Instance& instance, 
        struct wl_display* display, 
        struct wl_surface* surface, 
        uint32_t width, uint32_t height)
: m_surface(nullptr) // Initialize as null handle first
{
// The factory method is more reliable for platform-specific surfaces
vk::WaylandSurfaceCreateInfoKHR createInfo({}, display, surface);
    
    // Direct assignment: the returned RAII object is moved into m_surface
    m_surface = instance.createWaylandSurfaceKHR(createInfo);
    // 1. Select Physical Device with Scoring (Pick Discrete GPU)
    auto phys_devices = instance.enumeratePhysicalDevices();
    if (phys_devices.empty()) {
        throw std::runtime_error("No Vulkan GPUs found!");
    }
    auto phys_device = phys_devices[0];
    
    // 2. Queue Family Selection
    auto families = phys_device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            m_queue_family_index = i;
            break;
        }
    }

    // 3. Create Logical Device
    float priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_info({}, m_queue_family_index, 1, &priority);
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    
    vk::DeviceCreateInfo device_info({}, queue_info, {}, device_extensions);
    m_device = vk::raii::Device(phys_device, device_info);
    m_graphics_queue = vk::raii::Queue(m_device, m_queue_family_index, 0);

    // 4. Initial Setup
    create_sync_objects();
    create_command_resources();
    recreate_swapchain(width, height);
}

void Renderer::create_sync_objects() {
    vk::SemaphoreCreateInfo sem_info;
    vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_image_available_semaphores.emplace_back(m_device, sem_info);
        m_render_finished_semaphores.emplace_back(m_device, sem_info);
        m_in_flight_fences.emplace_back(m_device, fence_info);
    }
}

void Renderer::create_command_resources() {
    vk::CommandPoolCreateInfo pool_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_queue_family_index);
    m_command_pool = vk::raii::CommandPool(m_device, pool_info);

    vk::CommandBufferAllocateInfo alloc_info(*m_command_pool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT);
    m_command_buffers = vk::raii::CommandBuffers(m_device, alloc_info);
}

void Renderer::draw_frame() {
    // 1. THROTTLE: Wait for the GPU to finish this frame's previous iteration
    // This stops the 100% GPU utilization spin.
    if (m_device.waitForFences({*m_in_flight_fences[m_current_frame]}, true, UINT64_MAX) != vk::Result::eSuccess) return;

    // 2. ACQUIRE: Get image from swapchain
    uint32_t image_index;
    try {
        auto result = m_swapchain.acquireNextImage(UINT64_MAX, *m_image_available_semaphores[m_current_frame]);
        image_index = result.value;
    } catch (const vk::OutOfDateKHRError&) {
        return; // Signal resize needed
    }

    // 3. RESET: Now that we have the image, reset the fence for this frame's new work
    m_device.resetFences({*m_in_flight_fences[m_current_frame]});

    // 4. RECORD: Use the CommandBuffer for the current frame-in-flight
    auto& cmd = m_command_buffers[m_current_frame];
    cmd.reset();
    cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    
    // [OPTIONAL] Transition image layout here or clear screen
    
    cmd.end();

    // 5. SUBMIT:
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::SubmitInfo submit_info(*m_image_available_semaphores[m_current_frame], wait_stages, *cmd, *m_render_finished_semaphores[m_current_frame]);
    
    m_graphics_queue.submit(submit_info, *m_in_flight_fences[m_current_frame]);

    // 6. PRESENT:
    vk::PresentInfoKHR present_info(*m_render_finished_semaphores[m_current_frame], *m_swapchain, image_index);
    try {
        (void)m_graphics_queue.presentKHR(present_info);
    } catch (const vk::OutOfDateKHRError&) {
        // Handle resize
    }

    m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::recreate_swapchain(uint32_t width, uint32_t height) {
    m_device.waitIdle();
    
    // Triple Buffering: min + 1
    vk::SwapchainCreateInfoKHR info;
    info.setSurface(*m_surface); // Surface handle passed from window
    info.setMinImageCount(3); 
    info.setImageFormat(vk::Format::eB8G8R8A8Unorm); // Native for Wayland
    info.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    info.setImageExtent({width, height});
    info.setImageArrayLayers(1);
    info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    info.setPresentMode(vk::PresentModeKHR::eMailbox); // Fixes 165Hz jitter
    info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque); // Required for 'FLIP' path
    info.setOldSwapchain(*m_swapchain);

    m_swapchain = vk::raii::SwapchainKHR(m_device, info);
}

} // namespace Zeta