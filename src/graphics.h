#ifndef GRAPHICS_H
#define GRAPHICS_H

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vulkan/vulkan_core.h>

struct Frame
{
    VkFence fence;
    VkSemaphore acquireSemaphore;
    VkSemaphore releaseSemaphore;
    VkCommandPool pool;
    VkCommandBuffer buffer;
};

struct UniformConstants
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 camera;
    uint32_t frame;
};

struct Texture
{
    VkDeviceMemory memory;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkExtent2D extent;
    uint32_t mip_levels;
};

uint32_t find_memory_type(VkPhysicalDevice phy_device,
                          uint32_t filter_type,
                          VkMemoryPropertyFlags props);

VkShaderModule get_shader_module(const std::string path, VkShaderStageFlagBits bits);

void make_instance();

void make_device();

VkBuffer make_vertex_buffer(VkDeviceSize buffer_size, void *buffer_data, VkBufferUsageFlags flags);

void init_per_frame(int index);

void make_image(VkFormat format,
                VkImageUsageFlags usage,
                VkImageAspectFlags aspect,
                uint32_t width,
                uint32_t height,
                VkImage *image,
                VkImageView *view,
                void *data = nullptr);

void make_swapchain();

void make_pipeline();

void transition_image_layout(VkCommandBuffer cmd,
                             VkImage img,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout,
                             VkImageAspectFlags flags,
                             VkAccessFlags2 srcAccessMask,
                             VkAccessFlags2 dstAccessMask,
                             VkPipelineStageFlags2 srcStage,
                             VkPipelineStageFlags2 dstStage);

void render(uint32_t img, VkBuffer vertex_buffer, VkBuffer index_buffer, uint32_t index_count);

VkResult present_image(uint32_t index);

VkResult acquire_swapchain_image(uint32_t *img);

int g_main();

#endif // GRAPHICS_H
