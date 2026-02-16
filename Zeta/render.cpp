#define VK_USE_PLATFORM_WAYLAND_KHR
#include "Zeta/render.hpp"
#include <iostream>

#include <vulkan/vulkan_raii.hpp>
#include "xdg-shell-client-protocol.h"
namespace Zeta {

    Renderer::Renderer() {

}

void Renderer::init(wl_display* display, wl_surface* surface, uint32_t width, uint32_t height) {

    m_display = display;

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
    // 1. Pick Physical Device
    auto phys_devices = m_instance->enumeratePhysicalDevices();
    
    // Basic scoring to pick Discrete GPU (NVIDIA)
    for (auto& p : phys_devices) {
        if (p.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_physical_device.emplace(p);
            break;
        }
    }
    
    // Fallback if no dGPU found
    if (!m_physical_device) m_physical_device.emplace(phys_devices.front());

    // 2. Create Logical Device
    // Find a graphics queue family
    auto families = m_physical_device->getQueueFamilyProperties();
    uint32_t graphicsIndex = 0;
    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsIndex = i;
            break;
        }
    }

    float priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_info({}, graphicsIndex, 1, &priority);
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    
    vk::DeviceCreateInfo device_info({}, queue_info, {}, device_extensions);
    m_device.emplace(*m_physical_device, device_info);
}

void Renderer::create_graphics_queue() {
    m_graphics_queue.emplace(*m_device, m_queue_family_index, 0);
}

void Renderer::create_swapchain(uint32_t width, uint32_t height) {
    vk::SwapchainCreateInfoKHR info;
    info.setSurface(**m_surface);
    info.setMinImageCount(3);
    info.setImageFormat(vk::Format::eB8G8R8A8Unorm);
    info.setImageExtent({width, height});
    info.setImageArrayLayers(1);
    info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    info.setPresentMode(vk::PresentModeKHR::eMailbox);
    info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

    // 1. Create the RAII swapchain
    m_swapchain.emplace(*m_device, info);
    m_swapchain_images = m_swapchain->getImages();
    uint32_t image_count = static_cast<uint32_t>(m_swapchain_images.size());

    // Recreate semaphores to match the actual number of images
    m_image_available_sems.clear();
    m_render_finished_sems.clear();
    
    vk::SemaphoreCreateInfo sem_info;
    for (uint32_t i = 0; i < image_count; ++i) {
        m_image_available_sems.emplace_back(*m_device, sem_info);
        m_render_finished_sems.emplace_back(*m_device, sem_info);
    }
    
    m_images_in_flight.assign(image_count, nullptr);
}

void Renderer::create_sync_objects() {
    vk::SemaphoreCreateInfo sem_info;
    vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Use *m_device to pass the vk::raii::Device reference
        m_image_available_sems.emplace_back(*m_device, sem_info);
        m_render_finished_sems.emplace_back(*m_device, sem_info);
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

    // In Renderer.cpp / create_command_resources()
vk::BufferCreateInfo bufferInfo({}, 64, vk::BufferUsageFlagBits::eTransferDst);
m_dummy_buffer.emplace(*m_device, bufferInfo);

auto memReqs = m_dummy_buffer->getMemoryRequirements();
vk::MemoryAllocateInfo allocInfo(memReqs.size, findMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
m_dummy_memory.emplace(*m_device, allocInfo);

m_dummy_buffer->bindMemory(*m_dummy_memory, 0);
}

void Renderer::draw_frame() {
    // 1. Wait for the CPU-GPU sync fence for this 'slot' (0 or 1)
    if (m_device->waitForFences({*m_in_flight_fences[m_current_frame]}, true, UINT64_MAX) != vk::Result::eSuccess) return;

    // 2. Acquire Next Image
    uint32_t image_index;
    try {
        // Use the semaphore associated with the IMAGE slot
        // Note: You might need a temporary semaphore if you don't know the index yet,
        // but typically we use m_image_available_sems[m_current_frame] and then 
        // swap, but for your errors, indexing by image_index is the "Vulkan Guide" fix.
        
        // BETTER: Use a pool of semaphores for acquisition, but for now:
        auto result = m_swapchain->acquireNextImage(UINT64_MAX, *m_image_available_sems[m_current_frame]);
        image_index = result.value;
    } catch (const vk::OutOfDateKHRError&) { return; }

    // 3. Fence Tracking (Prevent colliding with a frame still rendering this image)
    if (m_images_in_flight[image_index]) {
        (void)m_device->waitForFences({m_images_in_flight[image_index]}, true, UINT64_MAX);
    }
    m_images_in_flight[image_index] = *m_in_flight_fences[m_current_frame];

    m_device->resetFences({*m_in_flight_fences[m_current_frame]});
    // 5. Record Commands
    auto& cmd = m_command_buffers[m_current_frame];
    cmd.reset();
    cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });


    cmd.fillBuffer(**m_dummy_buffer, 0, 64, 0);

    // Barrier to ensure the fill is recognized before presentation
    vk::BufferMemoryBarrier releaseBarrier(
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eMemoryRead,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        **m_dummy_buffer,
        0, 64
    );

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {}, nullptr, releaseBarrier, nullptr
    );

    // FIX: Mandatory Layout Transition (Even if drawing nothing)
    // If you use a RenderPass, the 'initialLayout' and 'finalLayout' do this for you.
    // If NOT using a RenderPass, you MUST use a Pipeline Barrier:
    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_swapchain_images[image_index]; // You need to store these in recreate_swapchain
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {}, nullptr, nullptr, barrier
    );

    cmd.end();

    // 4. Submit
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    
    vk::SubmitInfo submit_info(
        *m_image_available_sems[m_current_frame], // Wait for acquisition
        wait_stages, 
        *m_command_buffers[m_current_frame], 
        *m_render_finished_sems[image_index]     // Signal for this SPECIFIC image
    );
    
    m_graphics_queue->submit(submit_info, *m_in_flight_fences[m_current_frame]);

    // 5. Present
    vk::PresentInfoKHR present_info(
        *m_render_finished_sems[image_index],    // Wait for the specific image to finish
        **m_swapchain, 
        image_index
    );
    
    (void)m_graphics_queue->presentKHR(present_info);

    wl_display_flush(m_display); 

    m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void Renderer::recreate_swapchain(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return; // Prevent 0-extent error
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
    m_swapchain = vk::raii::SwapchainKHR(*m_device, info);

    m_swapchain_images = m_swapchain->getImages();


    // Recreate semaphores to match the actual number of images
    m_image_available_sems.clear();
    m_render_finished_sems.clear();
    
    vk::SemaphoreCreateInfo sem_info;
    for (uint32_t i = 0; i < m_swapchain_images.size(); ++i) {
        m_image_available_sems.emplace_back(*m_device, sem_info);
        m_render_finished_sems.emplace_back(*m_device, sem_info);
    }

    // 3. Reset the image-to-fence tracking vector
    // This must match the number of images in the new swapchain
    m_images_in_flight.assign(m_swapchain_images.size(), nullptr);
}



uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    auto memProperties = m_physical_device->getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

} // namespace Zeta