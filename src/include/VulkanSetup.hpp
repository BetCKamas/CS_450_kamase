#pragma once

///////////////////////////////////////////////////////////////////////////////
// Adapted from:
// - Vulkan tutorial: https://vulkan-tutorial.com/en/Drawing_a_triangle
// - Vulkan HPP samples: https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/samples 
///////////////////////////////////////////////////////////////////////////////

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <set>
#include <optional>
#include <array>
#include <limits>
#include <algorithm>
#include <fstream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// GLFW window creation
///////////////////////////////////////////////////////////////////////////////
GLFWwindow* createVulkanWindow(const char* windowName, int windowWidth, int windowHeight);
void cleanupVulkanWindow(GLFWwindow *window);

///////////////////////////////////////////////////////////////////////////////
// Vulkan instance 
///////////////////////////////////////////////////////////////////////////////
vk::Instance createVulkanInstance(const char *appName, const char *engineName, 
                                    vector<const char*> &validationLayers);
void cleanupVulkanInstance(vk::Instance instance);

///////////////////////////////////////////////////////////////////////////////
// Vulkan surface 
///////////////////////////////////////////////////////////////////////////////

vk::SurfaceKHR createVulkanSurface(vk::Instance instance, GLFWwindow *window);
void cleanupVulkanSurface(vk::Instance instance, vk::SurfaceKHR surface);

///////////////////////////////////////////////////////////////////////////////
// Vulkan physical devices
///////////////////////////////////////////////////////////////////////////////

void getAllVulkanPhysicalDevices(vk::Instance instance, vector<vk::PhysicalDevice>& allPhysicalDevices);
vk::PhysicalDevice pickFirstVulkanPhysicalDevice(vk::Instance instance);
//vk::PhysicalDevice pickBestVulkanPhysicalDevice(vk::Instance instance);
string getVulkanPhysicalDeviceName(vk::PhysicalDevice physicalDevice);

///////////////////////////////////////////////////////////////////////////////
// Vulkan queues
///////////////////////////////////////////////////////////////////////////////

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // If uninitialized, graphicsFamily.has_value() returns false
    std::optional<uint32_t> presentFamily;  // COULD be different from graphics family queue

    bool isComplete() {
        return (graphicsFamily.has_value()
                && presentFamily.has_value());
    }
};

bool isGraphicsQueue(vector<vk::QueueFamilyProperties> &queueFamilies, int queueIndex);
bool isPresentQueue(int queueIndex, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

///////////////////////////////////////////////////////////////////////////////
// Vulkan logical devices
///////////////////////////////////////////////////////////////////////////////

vk::Device createVulkanLogicalDevice(vk::Instance instance, vk::PhysicalDevice physicalDevice, 
                                    QueueFamilyIndices indices, vector<const char*> &validationLayers);
void cleanupVulkanLogicalDevice(vk::Device &device);

///////////////////////////////////////////////////////////////////////////////
// Vulkan swap chain
///////////////////////////////////////////////////////////////////////////////

struct SwapChainData {
    vk::SwapchainKHR chain;
    vk::Format format;
    vk::Extent2D extents;
    vector<vk::Image> images;
    vector<vk::ImageView> views;
};

vk::SurfaceFormatKHR pickSurfaceFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
vk::Extent2D getSurfaceExtents(GLFWwindow *window, const vk::SurfaceCapabilitiesKHR& cap);
vk::PresentModeKHR pickSwapPresentMode(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

SwapChainData createSwapChainData(GLFWwindow *window, vk::PhysicalDevice physicalDevice, 
                                    vk::Device &device,
                                    vk::SurfaceKHR surface, QueueFamilyIndices indice);
void cleanupSwapChainData(vk::Device &device, SwapChainData& swapChainData);

///////////////////////////////////////////////////////////////////////////////
// Vulkan render pass
///////////////////////////////////////////////////////////////////////////////

vk::RenderPass createVulkanRenderPass(vk::Device &device, SwapChainData &swapChainData);
void cleanupVulkanRenderPass(vk::Device &device, vk::RenderPass &pass);

///////////////////////////////////////////////////////////////////////////////
// Vulkan vertex buffer
///////////////////////////////////////////////////////////////////////////////

struct VulkanVertex {
    glm::vec3 pos;
    glm::vec3 color;    
};

vk::VertexInputBindingDescription getVertexBindingDescription();
vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();

///////////////////////////////////////////////////////////////////////////////
// Vulkan pipeline
///////////////////////////////////////////////////////////////////////////////

struct PipelineData {
    vk::PipelineCache cache;
    vk::PipelineLayout pipelineLayout; // Necessary for passing in uniform variables
    vk::Pipeline graphicsPipeline;
};

static std::vector<char> readBinaryFile(const std::string& filename);
vk::ShaderModule createShaderModule(vk::Device &device, const std::vector<char>& code);
PipelineData createGraphicsPipelineData(  vk::Device &device,
                                            SwapChainData &swapChainData,
                                            vk::RenderPass &renderPass, 
                                            string vertSPVFilename, 
                                            string fragSPVFilename);
void cleanupGraphicsPipelineData(vk::Device &device, PipelineData &data);

///////////////////////////////////////////////////////////////////////////////
// Vulkan framebuffers
///////////////////////////////////////////////////////////////////////////////

vector<vk::Framebuffer> createVulkanFramebuffers(   vk::Device &device, 
                                                    SwapChainData &swapChainData,
                                                    vk::RenderPass &renderPass);
void cleanupVulkanFramebuffers(vk::Device &device, vector<vk::Framebuffer> &framebuffers);

///////////////////////////////////////////////////////////////////////////////
// Vulkan command pools and buffers
///////////////////////////////////////////////////////////////////////////////

vk::CommandPool createVulkanCommandPool(vk::Device &device, QueueFamilyIndices &indices);
vk::CommandBuffer createVulkanCommandBuffer(vk::Device &device, vk::CommandPool &commandPool);
void cleanupVulkanCommandPool(vk::Device &device, vk::CommandPool &pool);

///////////////////////////////////////////////////////////////////////////////
// Vulkan sync objects
///////////////////////////////////////////////////////////////////////////////

vk::Semaphore createVulkanSemaphore(vk::Device &device);
vk::Fence createVulkanFence(vk::Device &device);
void cleanupVulkanSemaphore(vk::Device &device, vk::Semaphore &s);
void cleanupVulkanFence(vk::Device &device, vk::Fence &f);

///////////////////////////////////////////////////////////////////////////////
// Vulkan mesh
///////////////////////////////////////////////////////////////////////////////

struct HostMesh {
    vector<VulkanVertex> vertices {};
    vector<uint16_t> indices {};
};

struct VulkanBuffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
};

struct VulkanMesh {
    VulkanBuffer vertices;
    VulkanBuffer indices;
    int indexCnt = 0;
};

uint32_t findMemoryType(vk::PhysicalDevice &physicalDevice, uint32_t typeFilter, 
                        vk::MemoryPropertyFlags properties);
VulkanBuffer createVulkanBuffer(vk::PhysicalDevice &physicalDevice,
                                vk::Device &device,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties);
void copyDataToVulkanBuffer(vk::Device &device, vk::DeviceMemory memory, size_t bufferSize, void *hostData);

VulkanMesh createVulkanMesh(vk::PhysicalDevice &physicalDevice,
                            vk::Device &device,
                            HostMesh &hostMesh);   

void drawVulkanMesh(vk::CommandBuffer &commandBuffer, VulkanMesh &mesh);                      

void cleanupVulkanBuffer(vk::Device &device, VulkanBuffer &data);
void cleanupVulkanMesh(vk::Device &device, VulkanMesh &mesh);
