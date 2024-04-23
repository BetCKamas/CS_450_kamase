#include "VulkanSetup.hpp"

///////////////////////////////////////////////////////////////////////////////
// Adapted from:
// - Vulkan tutorial: https://vulkan-tutorial.com/en/Drawing_a_triangle
// - Vulkan HPP samples: https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/samples 
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// GLFW window creation
///////////////////////////////////////////////////////////////////////////////

GLFWwindow* createVulkanWindow(const char *windowName, int windowWidth, int windowHeight) {
        // Initialize GLFW as usual
        glfwInit();

        // Do NOT create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Disable resizing for now
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Create window
        GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, windowName, nullptr, nullptr);

        // Return window
        return window;
}

void cleanupVulkanWindow(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan instance 
///////////////////////////////////////////////////////////////////////////////

vk::Instance createVulkanInstance(const char *appName, const char *engineName, 
                                    vector<const char*> &validationLayers) {
    // Prepare list of extensions
    // We need:
    // - Extensions requested by GLFW
    // - Possibly one more for Mac compatibility (of course)
    vector<const char*> requiredExtensions;
    
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for(int i = 0; i < (int)glfwExtensionCount; i++) {
        requiredExtensions.push_back(glfwExtensions[i]); 
    }

    vk::InstanceCreateFlags creationFlags {};

    // MAC ONLY: Add compatability extension
    // requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    // creationFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
        
    // Create application info
    vk::ApplicationInfo appInfo {appName, 1, engineName};

    // Set up instance creation parameters
    vk::InstanceCreateInfo createInfo(
        creationFlags,
        &appInfo,
        static_cast<uint32_t>(validationLayers.size()), 
        validationLayers.data(),
        static_cast<uint32_t>(requiredExtensions.size()),
        requiredExtensions.data());
             
    // Create and return Vulkan instance
    vk::Instance instance = vk::createInstance(createInfo);

    return instance;
}

void cleanupVulkanInstance(vk::Instance instance) {
    instance.destroy();    
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan surface 
///////////////////////////////////////////////////////////////////////////////

vk::SurfaceKHR createVulkanSurface(vk::Instance instance, GLFWwindow *window) {
    VkSurfaceKHR old_surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &old_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    
    return vk::SurfaceKHR(old_surface);
}

void cleanupVulkanSurface(vk::Instance instance, vk::SurfaceKHR surface) {
    instance.destroySurfaceKHR(surface);
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan physical devices
///////////////////////////////////////////////////////////////////////////////

void getAllVulkanPhysicalDevices(vk::Instance instance, vector<vk::PhysicalDevice>& allPhysicalDevices) {
    allPhysicalDevices = instance.enumeratePhysicalDevices();
}

vk::PhysicalDevice pickFirstVulkanPhysicalDevice(vk::Instance instance) {
    // Get all physical devices
    vector<vk::PhysicalDevice> allPhysicalDevices;
    getAllVulkanPhysicalDevices(instance, allPhysicalDevices);
    // Do we HAVE a device?
    if(allPhysicalDevices.size() == 0) {
        throw std::runtime_error("Failed to find ANY suitable GPU!");
    }

    // Grab first one
    return allPhysicalDevices.front();
}

string getVulkanPhysicalDeviceName(vk::PhysicalDevice physicalDevice) {
    vk::PhysicalDeviceProperties properties;
    physicalDevice.getProperties(&properties);
    return properties.deviceName;
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan queues
///////////////////////////////////////////////////////////////////////////////

bool isGraphicsQueue(vector<vk::QueueFamilyProperties> &queueFamilies, int queueIndex) {
    auto valid = (queueFamilies[queueIndex].queueFlags & vk::QueueFlagBits::eGraphics);
    return (bool)valid;
}

bool isPresentQueue(int queueIndex, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {    
    return physicalDevice.getSurfaceSupportKHR(queueIndex, surface);     
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {    
    // Get all the queue families available
    // (Each one handles different kinds of operations, e.g., graphics vs. compute)
    vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

    // We need one for graphics and presentation.
    // Often these are the same, BUT they may not be

    QueueFamilyIndices indices;
    for(int i = 0; i < queueFamilies.size(); i++) {
        if(isGraphicsQueue(queueFamilies, i)) {
            indices.graphicsFamily = static_cast<uint32_t>(i);
        }

        if(isPresentQueue(i, physicalDevice, surface)) {
            indices.presentFamily = static_cast<uint32_t>(i);            
        }

        if(indices.isComplete()) {
            break;
        }
    }

    // If we didn't find the queues we want..
    if(!indices.isComplete()) {
        // Find one for each...
        throw std::runtime_error("Could not find appropriate queues!");
    }

    return indices;
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan logical devices
///////////////////////////////////////////////////////////////////////////////

vk::Device createVulkanLogicalDevice(vk::Instance instance, vk::PhysicalDevice physicalDevice, 
                                    QueueFamilyIndices indices, vector<const char*> &validationLayers) {
    // For device extensions, we just need swap chain support
    vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // Set up queue creation info
    float queuePriority = 1.0f;
    vector<vk::DeviceQueueCreateInfo> queueCreateInfo = {
        vk::DeviceQueueCreateInfo({}, indices.graphicsFamily.value(), 1, &queuePriority)        
    };

    // Only add presentation queue if different
    if(indices.graphicsFamily.value() != indices.presentFamily.value()) {
        queueCreateInfo.push_back(vk::DeviceQueueCreateInfo({}, indices.presentFamily.value(), 1, &queuePriority));
    }

    // Create LOGICAL device
    vk::DeviceCreateInfo deviceCreateInfo({}, queueCreateInfo, validationLayers, extensions);
    vk::Device device = physicalDevice.createDevice(deviceCreateInfo);

    return device;
}

void cleanupVulkanLogicalDevice(vk::Device &device) {
    device.destroy();
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan swap chain
///////////////////////////////////////////////////////////////////////////////

vk::SurfaceFormatKHR pickSurfaceFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
    // Get the possible formats
    vector<vk::SurfaceFormatKHR> allSurfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);

    // Prefer 32-bit BGRA (commented version is sRGB color space (non-linear RGB))
    for (const auto& availableFormat : allSurfaceFormats) {        
        //vk::Format::eB8G8R8A8Unorm
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm //vk::Format::eB8G8R8A8Srgb
            && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
        }
    }

    // If not, just return first one
    return allSurfaceFormats[0];
}


vk::Extent2D getSurfaceExtents(GLFWwindow *window, const vk::SurfaceCapabilitiesKHR& cap) {
    // If there is a difference between SCREEN coordinates and PIXEL coordinates (e.g., Retina displays),
    // then currentExtent values set to max (and we need to use glfwGetFramebufferSize())
    // Otherwise, they are correct.
    if (cap.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return cap.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = clamp(actualExtent.width, cap.minImageExtent.width, cap.maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height, cap.minImageExtent.height, cap.maxImageExtent.height);

        return actualExtent;
    }
}

vk::PresentModeKHR pickSwapPresentMode(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
    // Get all presentation modes
    vector<vk::PresentModeKHR> allPresentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    
    // Prefer triple-buffering...
    for (const auto& mode : allPresentModes) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        }
    }

    // Otherwise, double-buffering with V-Sync
    return vk::PresentModeKHR::eFifo; 
}

SwapChainData createSwapChainData(GLFWwindow *window, vk::PhysicalDevice physicalDevice, 
                                    vk::Device &device,
                                    vk::SurfaceKHR surface, QueueFamilyIndices indices) {
    SwapChainData data;

    // Get surface format
    vk::SurfaceFormatKHR surfaceFormat = pickSurfaceFormat(physicalDevice, surface);

    // Get surface capabilities
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

    // Get the extents of the surface
    vk::Extent2D surfaceExtents = getSurfaceExtents(window, surfaceCapabilities);

    // Get the presentation mode  
    vk::PresentModeKHR presentMode = pickSwapPresentMode(physicalDevice, surface);
    
    // To avoid having to wait, ask for one more image than the minimum
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    // Create actual swap chain
    vk::SwapchainCreateInfoKHR createInfo(
        {},
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        surfaceExtents,
        1,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,    // Assuming only ONE queue that's shared for graphics and present
        {},
        surfaceCapabilities.currentTransform, // Pretransform: use default
        vk::CompositeAlphaFlagBitsKHR::eOpaque, // Ignore alpha channel for blending
        presentMode,
        true, // Don't care about pixels obscured by another window (problem if need to read these pixels)
        nullptr // If window is resized, need to recreate swap chain and old one specified (ignoring all this for now)
    );
    
    // Define how swap chain used across multiple queue families   
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } 

    // NOW create swap chain    
    data.chain = device.createSwapchainKHR(createInfo);

    // Store other data for later
    data.extents = surfaceExtents;
    data.format = surfaceFormat.format;
    
    // Get actual swap chain images
    data.images = device.getSwapchainImagesKHR(data.chain);
    
    // IMAGE VIEWS
    // Need a view for each image
    data.views.resize(data.images.size());

    vk::ImageViewCreateInfo imageViewCreateInfo(
        {},
        {},
        vk::ImageViewType::e2D,
        surfaceFormat.format,
        {},
        {
            vk::ImageAspectFlagBits::eColor,
            0, // baseMipLevel
            1, // levelCount
            0, // baseArrayLayer
            1  // layerCount
        }
    );
    
    for (size_t i = 0; i < data.images.size(); i++) {
        imageViewCreateInfo.image = data.images[i];
        data.views[i] = device.createImageView(imageViewCreateInfo);        
    }

    return data;
}

void cleanupSwapChainData(vk::Device &device, SwapChainData& swapChainData) {
    for (auto imageView : swapChainData.views) {
        device.destroyImageView(imageView);
    }
    swapChainData.views.clear();
    swapChainData.images.clear();
    device.destroySwapchainKHR(swapChainData.chain);    
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan render pass
///////////////////////////////////////////////////////////////////////////////

vk::RenderPass createVulkanRenderPass(vk::Device &device, SwapChainData &swapChainData) {

    // Create attachment for color and depth
    vector<vk::AttachmentDescription> attachmentDescriptions;

    // Color first
    attachmentDescriptions.push_back(vk::AttachmentDescription(
        {},
        swapChainData.format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,       // Clear buffer to constant value on load
        vk::AttachmentStoreOp::eStore,      // Store values (so we can see what we render :)
        vk::AttachmentLoadOp::eDontCare,    // Don't care about stencil buffer
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,        // Initially undefined before presentation
        vk::ImageLayout::ePresentSrcKHR     // Present appropriate to surface
    ));

    /*
    // Depth attachment
    attachmentDescriptions.push_back(vk::AttachmentDescription(
        {},
        depthFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,       // Clear buffer to constant value on load
        vk::AttachmentStoreOp::eDontCare,   // We don't need to see this later
        vk::AttachmentLoadOp::eDontCare,    // Don't care about stencil buffer
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,        // Initially undefined before presentation
        vk::ImageLayout::eDepthStencilAttachmentOptimal     // Present as depth buffer
    ));*/

    // Which images to attach to this subpass
    // Refers to: layout(location = 0) out vec4 outColor
    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    //vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
   
    // Describe the actual subpass
    vk::SubpassDescription subpassDescription(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        {},
        colorAttachmentRef,
        {},
        nullptr //depthAttachmentRef
    );  

    // Make the ACTUAL render pass
    return device.createRenderPass(vk::RenderPassCreateInfo(
        {},
        attachmentDescriptions,
        subpassDescription
    ));    
}

void cleanupVulkanRenderPass(vk::Device &device, vk::RenderPass &pass) {
    device.destroyRenderPass(pass);
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan vertex buffer
///////////////////////////////////////////////////////////////////////////////

vk::VertexInputBindingDescription getVertexBindingDescription() {
    return vk::VertexInputBindingDescription(0, sizeof(VulkanVertex), vk::VertexInputRate::eVertex);    
}

vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
    vector<vk::VertexInputAttributeDescription> attributeDescriptions;

    // POSITION
    attributeDescriptions.push_back(vk::VertexInputAttributeDescription(
        0, // location
        0, // binding
        vk::Format::eR32G32B32Sfloat,  // format
        offsetof(VulkanVertex, pos) // offset
    ));

    // COLOR
    attributeDescriptions.push_back(vk::VertexInputAttributeDescription(
        1, // location
        0, // binding
        vk::Format::eR32G32B32Sfloat,  // format
        offsetof(VulkanVertex, color) // offset
    ));

    return attributeDescriptions;
};

///////////////////////////////////////////////////////////////////////////////
// Vulkan pipeline
///////////////////////////////////////////////////////////////////////////////

static std::vector<char> readBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

vk::ShaderModule createShaderModule(vk::Device &device, const std::vector<char>& code) {

    return device.createShaderModule(vk::ShaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(), code.size(), 
        reinterpret_cast<const uint32_t*>(code.data()) // Cast that pretends as if it were a uint32_t pointer
    ));
}

PipelineData createGraphicsPipelineData(vk::Device &device,
                                        SwapChainData &swapChainData,
                                        vk::RenderPass &renderPass, 
                                        string vertSPVFilename, 
                                        string fragSPVFilename) {

    // Set up data
    PipelineData data;

    // Load up BYTECODE shader files
    auto vertShaderCode = readBinaryFile(vertSPVFilename);
    auto fragShaderCode = readBinaryFile(fragSPVFilename);

    std::cout << "Size of shader code loaded: " << vertShaderCode.size() << " " << fragShaderCode.size() << std::endl;

    // Compiling/linking to GPU machine code doesn't happen until graphics pipeline created.
    // Once the pipeline is created, we will be able to destroy these modules safely.
    vk::ShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    // Assign VERTEX SHADER to appropriate stage
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main");  
   
    // Assign FRAGMENT SHADER to appropriate stage
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");  

    // Combine them
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Get how vertex attribute data is organized
    auto bindingDescription = getVertexBindingDescription();
    auto attributeDescriptions = getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, bindingDescription, attributeDescriptions);
        
    // Render a regular triangle list
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);
    
    // Set viewport (startx, starty, width, height, mindepth, maxdepth)
    vk::Viewport viewport(0, 0, (float)swapChainData.extents.width, (float)swapChainData.extents.height, 0.0f, 1.0f);

    // Set scissors (if we wanted to only draw part of the screen)
    // Using full screen here
    vk::Rect2D scissor({0,0}, swapChainData.extents);
          
    // Identify properties that you want to be able to change dynamically
    // WITHOUT recreating the whole pipeline
    vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor        
    };

    vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStates);  

    vk::PipelineViewportStateCreateInfo viewportState({}, viewport, scissor);
    
    // Set RASTERIZER stage values
    // Defaults to:
    // - discard fragments outside near/far planes
    // - polygon fill mode
    

    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    // Change the following:
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise; 
                
    // Set up BLENDING
    // Basically no blending here
    // Per frame buffer...
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    
    // Global blend settings
    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, colorBlendAttachment);
    
    // Create empty layout for now (will need later for uniform variables)
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},
        0,          // setLayoutCount
        nullptr,    // pSetLayouts
        0,          // pushConstantRangeCount
        nullptr     // pPushConstantRanges 
    );
    
    // Create the pipeline layout
    data.pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    // Not doing multisample AA
    vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);

    // Create pipeline cache
    data.cache = device.createPipelineCache( vk::PipelineCacheCreateInfo());

    // CREATE ACTUAL PIPELINE
    vk::GraphicsPipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(),
                                                shaderStages,
                                                &vertexInputInfo,
                                                &inputAssembly,
                                                0,
                                                &viewportState,
                                                &rasterizer,
                                                &multisample,
                                                0,
                                                &colorBlending,
                                                &dynamicState,
                                                data.pipelineLayout,
                                                renderPass);    
    
    auto ret = device.createGraphicsPipeline(data.cache, pipelineInfo);

    if (ret.result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // Set pipeline
    data.graphicsPipeline = ret.value;

    // Cleanup modules
    device.destroyShaderModule(fragShaderModule);
    device.destroyShaderModule(vertShaderModule);
    
    // Return data
    return data;
}

void cleanupGraphicsPipelineData(vk::Device &device, PipelineData &data) {
    device.destroyPipelineCache(data.cache);
    device.destroyPipelineLayout(data.pipelineLayout);
    device.destroyPipeline(data.graphicsPipeline);
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan framebuffers
///////////////////////////////////////////////////////////////////////////////

vector<vk::Framebuffer> createVulkanFramebuffers(   vk::Device &device, 
                                                    SwapChainData &swapChainData,
                                                    vk::RenderPass &renderPass) {
    // First allocate space
    vector<vk::Framebuffer> framebuffers;
    framebuffers.resize(swapChainData.views.size());
    
    // For each image view...
    for (size_t i = 0; i < swapChainData.views.size(); i++) {
        vector<vk::ImageView> attachments = {swapChainData.views.at(i)};
        framebuffers[i] = device.createFramebuffer(vk::FramebufferCreateInfo({}, renderPass, attachments, 
                                                    swapChainData.extents.width, 
                                                    swapChainData.extents.height, 1));
    }

    return framebuffers;
}

void cleanupVulkanFramebuffers(vk::Device &device, vector<vk::Framebuffer> &framebuffers) {
    for (auto framebuffer : framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    framebuffers.clear();        
}
                                        
///////////////////////////////////////////////////////////////////////////////
// Vulkan command pools and buffers
///////////////////////////////////////////////////////////////////////////////

vk::CommandPool createVulkanCommandPool(vk::Device &device, QueueFamilyIndices &indices) {

    return device.createCommandPool(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), // Reset pool every frame
            indices.graphicsFamily.value()));   
}

vk::CommandBuffer createVulkanCommandBuffer(vk::Device &device, vk::CommandPool &commandPool) {
    return device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1)).front();    
}

void cleanupVulkanCommandPool(vk::Device &device, vk::CommandPool &commandPool) {
    device.destroyCommandPool(commandPool);
}
    

///////////////////////////////////////////////////////////////////////////////
// Vulkan sync objects
///////////////////////////////////////////////////////////////////////////////

vk::Semaphore createVulkanSemaphore(vk::Device &device) {
    return device.createSemaphore(vk::SemaphoreCreateInfo());
}

vk::Fence createVulkanFence(vk::Device &device) {
    return device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
}

void cleanupVulkanSemaphore(vk::Device &device, vk::Semaphore &s) {
    device.destroySemaphore(s);
}

void cleanupVulkanFence(vk::Device &device, vk::Fence &f) {
    device.destroyFence(f);
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan mesh
///////////////////////////////////////////////////////////////////////////////

uint32_t findMemoryType(vk::PhysicalDevice &physicalDevice, uint32_t typeFilter, 
                        vk::MemoryPropertyFlags properties) {

    // Get memory properties of physical device
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VulkanBuffer createVulkanBuffer(vk::PhysicalDevice &physicalDevice,
                                vk::Device &device,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties) {

    // Set up struct
    VulkanBuffer data;

    // Create buffer (memory not allocated YET)
    data.buffer = device.createBuffer(vk::BufferCreateInfo(vk::BufferCreateFlags(), size, usage, vk::SharingMode::eExclusive));
          
    // Get memory requirements
    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(data.buffer);

    // Set up allocation info
    vk::MemoryAllocateInfo allocInfo(
        memRequirements.size, 
        findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties));
        
    // Actually allocate memory
    data.memory = device.allocateMemory(allocInfo);

    // Bind the memory
    device.bindBufferMemory(data.buffer, data.memory, 0);

    // Return data
    return data;
}

void copyDataToVulkanBuffer(vk::Device &device, vk::DeviceMemory memory, size_t bufferSize, void *hostData) {
    void* data;
    vkMapMemory(device, memory, 0, bufferSize, 0, &data);
        memcpy(data, hostData, (size_t) bufferSize);
    vkUnmapMemory(device, memory);
}

VulkanMesh createVulkanMesh(vk::PhysicalDevice &physicalDevice,
                            vk::Device &device, HostMesh &hostMesh) {
    // Set up Vulkan mesh                            
    VulkanMesh mesh;

    // Create vertex buffer
    vk::DeviceSize vertBufferSize = sizeof(hostMesh.vertices[0]) * hostMesh.vertices.size();
    mesh.vertices = createVulkanBuffer(
        physicalDevice, device, vertBufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // Copy in data
    copyDataToVulkanBuffer(device, mesh.vertices.memory, vertBufferSize, hostMesh.vertices.data());      

    // Create index buffer
    vk::DeviceSize indexBufferSize = sizeof(hostMesh.indices[0]) * hostMesh.indices.size();
    mesh.indices = createVulkanBuffer(
        physicalDevice, device, indexBufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer, 
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // Copy in data
    copyDataToVulkanBuffer(device, mesh.indices.memory, indexBufferSize, hostMesh.indices.data());

    // Set index count
    mesh.indexCnt = hostMesh.indices.size();

    // Return mesh
    return mesh;
}

void drawVulkanMesh(vk::CommandBuffer &commandBuffer, VulkanMesh &mesh) {
    
    vk::Buffer vertexBuffers[] = {mesh.vertices.buffer};
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(mesh.indices.buffer, 0, vk::IndexType::eUint16);
    
    commandBuffer.drawIndexed(static_cast<uint32_t>(mesh.indexCnt), 1, 0, 0, 0);
}    

void cleanupVulkanBuffer(vk::Device &device, VulkanBuffer &data) {
    device.destroyBuffer(data.buffer);
    device.freeMemory(data.memory);
}

void cleanupVulkanMesh(vk::Device &device, VulkanMesh &mesh) {
    cleanupVulkanBuffer(device, mesh.vertices);
    cleanupVulkanBuffer(device, mesh.indices);
}
