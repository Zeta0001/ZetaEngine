#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "Zeta/render.hpp"
#include <iostream>

#include <vulkan/vulkan_raii.hpp>
#include "xdg-shell-client-protocol.h"

#include <fstream>

namespace Zeta {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, // Added vk::
        vk::DebugUtilsMessageTypeFlagsEXT messageTypes,          // Added vk::
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, // Added vk::
        void* pUserData) {
        
        std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    std::vector<uint32_t> load_spirv(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open " + filename);
    
        size_t fileSize = (size_t)file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        return buffer;
    }

Renderer::Renderer() : 
    m_context(),
    m_instance(nullptr),
    m_surface(nullptr),
    m_physicalDevice(nullptr),
    m_device(nullptr),
    m_graphicsQueue(nullptr),
    m_swapchain(nullptr),
    m_commandPool(nullptr),
    m_commandBuffers(nullptr)
{

}

void Renderer::init(wl_display* display, wl_surface* surface, uint32_t width, uint32_t height) {

    m_instance = create_instance();
    m_surface = create_surface(display, surface);
    m_physicalDevice = create_physical_device();
    m_device = create_logical_device();
    m_graphicsQueue = create_graphics_queue();
    m_swapchain = create_swapchain(width, height);
    m_commandPool = create_command_pool();
    m_commandBuffers = create_command_buffers();

    // 4. Initial Setup
    m_swapchainImages = m_swapchain.getImages();
    // 3. Create RAII ImageViews
    m_swapchainImageViews.clear();
    m_swapchainImageViews.reserve(m_swapchainImages.size());

    for (auto image : m_swapchainImages) {
        vk::ImageViewCreateInfo viewInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = m_swapchainFormat,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        };
        m_swapchainImageViews.emplace_back(m_device, viewInfo);
    }
    create_sync_objects();

    create_graphics_pipeline();
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

    std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
    // 1. Define the required extensions for Wayland
    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,         // "VK_KHR_surface"
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME  // "VK_KHR_wayland_surface"
    };

    // 2. Early Messenger (for instance create/destroy validation)
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | 
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | 
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = debugCallback
    };
    // 2. Set up the Instance creation info
    vk::InstanceCreateInfo createInfo{
        .pNext = &debugInfo, // Capture instance creation errors
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    // 3. Return the RAII Instance
    // Note: m_context must be a vk::raii::Context
    return vk::raii::Instance(m_context, createInfo);
}

vk::raii::SurfaceKHR Renderer::create_surface(wl_display* display, wl_surface* surface) {
        // Creation info for the persistent messenger
        vk::DebugUtilsMessengerCreateInfoEXT debugInfo{
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | 
                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | 
                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = debugCallback
        };

        m_debugMessenger = vk::raii::DebugUtilsMessengerEXT(m_instance, debugInfo);


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

vk::raii::Device Renderer::create_logical_device() {
    // 1. Setup Features (Vulkan 1.3 style)
    vk::PhysicalDeviceVulkan13Features features13{
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE
    };

    vk::PhysicalDeviceVulkan12Features features12{
        .pNext = &features13,
        .timelineSemaphore = VK_TRUE
    };

    vk::PhysicalDeviceVulkan11Features features11{
        .pNext = &features12,
        .shaderDrawParameters = VK_TRUE // Fixed your SPIR-V error
    };

    // 2. Queue and Extension setup
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = m_queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // 3. Create Device
    vk::DeviceCreateInfo createInfo{
        .pNext = &features11, // Link the feature chain here!
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    return vk::raii::Device(m_physicalDevice, createInfo);
}

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

    m_swapchainFormat = surfaceFormat.format; 
    m_swapchainExtent = extent;
    static int count = 0;
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

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_imageAvailableSemaphores.emplace_back(m_device, binaryInfo);
    }

    uint32_t imageCount = m_swapchainImages.size(); 
    m_renderFinishedSemaphores.clear();
    for (uint32_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo{});
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
    auto [result, imageIndex] = m_swapchain.acquireNextImage(UINT64_MAX, *m_imageAvailableSemaphores[syncIndex]);

    // 3. COMMAND RECORDING
    auto& cmd = m_commandBuffers[syncIndex];
    vk::Image image = m_swapchainImages[imageIndex];

    cmd.reset();
    cmd.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    // 1. Transition Image: Undefined -> Color Attachment
    vk::ImageMemoryBarrier2 barrier_to_render{
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .image = image,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{ .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier_to_render });

    // 2. Begin Dynamic Rendering
    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = *m_swapchainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearColorValue(std::array<float, 4>{1.0f, 0.5f, 0.0f, 1.0f}) // Orange
    };

    vk::RenderingInfo renderingInfo{
        .renderArea = { .offset = {0, 0}, .extent = m_swapchainExtent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment
    };

    cmd.beginRendering(renderingInfo);
    
    // 3. Bind and Draw
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);
    
    // Set dynamic viewport/scissor (since we enabled them in the pipeline)
    cmd.setViewport(0, vk::Viewport{0.0f, 0.0f, (float)m_swapchainExtent.width, (float)m_swapchainExtent.height, 0.0f, 1.0f});
    cmd.setScissor(0, vk::Rect2D{{0, 0}, m_swapchainExtent});
    cmd.draw(3, 1, 0, 0); // Our triangle!

    cmd.endRendering();

    // 4. Transition Image: Color Attachment -> Present
    vk::ImageMemoryBarrier2 barrier_to_present = barrier_to_render;
    barrier_to_present.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier_to_present.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier_to_present.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier_to_present.newLayout = vk::ImageLayout::ePresentSrcKHR;
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
            .semaphore = *m_renderFinishedSemaphores[imageIndex],
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
        .pWaitSemaphores = &(*m_renderFinishedSemaphores[imageIndex]),
        .swapchainCount = 1,
        .pSwapchains = &(*m_swapchain),
        .pImageIndices = &imageIndex
    };

    // presentKHR returns a vk::Result
    auto presentResult = m_graphicsQueue.presentKHR(presentInfo);
}


void Renderer::recreate_swapchain(uint32_t width, uint32_t height) {
}



void Renderer::create_graphics_pipeline() {
    // 1. Create Shader Modules
    auto vertCode = load_spirv("shaders/triangle.vert.spv");
    auto fragCode = load_spirv("shaders/triangle.frag.spv");

    vk::raii::ShaderModule vertModule(m_device, vk::ShaderModuleCreateInfo{
        .codeSize = vertCode.size() * sizeof(uint32_t),
        .pCode = vertCode.data()
    });

    vk::raii::ShaderModule fragModule(m_device, vk::ShaderModuleCreateInfo{
        .codeSize = fragCode.size() * sizeof(uint32_t),
        .pCode = fragCode.data()
    });

    // 2. Define Shader Stages
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {{
        { .stage = vk::ShaderStageFlagBits::eVertex, .module = *vertModule, .pName = "main" },
        { .stage = vk::ShaderStageFlagBits::eFragment, .module = *fragModule, .pName = "main" }
    }};

    // 3. Fixed Function States (Minimum for Triangle)
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };
    
    // Use dynamic viewport/scissor to avoid recreating pipeline on resize
    std::array<vk::DynamicState, 2> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1 };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | 
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .attachmentCount = 1, .pAttachments = &colorBlendAttachment
    };

    // 4. Create Pipeline Layout
    m_pipelineLayout = vk::raii::PipelineLayout(m_device, vk::PipelineLayoutCreateInfo{});

    vk::PipelineRenderingCreateInfo renderingInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_swapchainFormat // e.g., vk::Format::eB8G8R8A8Srgb
    };
    // 5. Create Graphics Pipeline
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &renderingInfo, // Attach format info here
        .stageCount = 2,
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = *m_pipelineLayout,
        .renderPass = nullptr // Note: Use VK_KHR_dynamic_rendering if no RenderPass
    };

    m_graphicsPipeline = vk::raii::Pipeline(m_device, nullptr, pipelineInfo);
}


} // namespace Zeta