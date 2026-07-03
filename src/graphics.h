#ifndef GRAPHICS_H
#define GRAPHICS_H

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include "mesh_loader.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vulkan/vulkan_core.h>

class Pass;

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

struct TextureDescription
{
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageAspectFlags aspect;
};

struct BufferData
{
    void *data;
    size_t size;
};

struct BufferDescription
{
    VkDeviceSize buffer_size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memProperty;
};

struct Buffer
{
    VkDeviceMemory memory;
    VkBuffer buffer;
    void *mapped_data;
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

struct NativeModel
{
    Buffer vertex;
    Buffer index;
    Texture texture;
    uint32_t index_count;
    vec3 position = {0, 0, 0};
};

uint32_t find_memory_type(VkPhysicalDevice phy_device,
                          uint32_t filter_type,
                          VkMemoryPropertyFlags props);

VkShaderModule get_shader_module(const std::string path, VkShaderStageFlagBits bits);

void make_instance();

void make_device();

Buffer make_buffer(BufferDescription desc, void *data = nullptr);

void init_per_frame(int index);

Texture make_image(TextureDescription desc, Image *image = nullptr);

void make_swapchain();

void make_render_target();

void transition_image_layout(VkCommandBuffer cmd,
                             VkImage img,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout,
                             VkImageAspectFlags flags,
                             VkAccessFlags2 srcAccessMask,
                             VkAccessFlags2 dstAccessMask,
                             VkPipelineStageFlags2 srcStage,
                             VkPipelineStageFlags2 dstStage);

void render(uint32_t img, std::vector<Pass *> graphicsPasses, std::vector<Pass *> computePasses);

VkResult present_image(uint32_t index);

VkResult acquire_swapchain_image(uint32_t *img);

NativeModel make_native_model(Model &model);

void begin_render_loop(std::vector<Pass *> &graphicsPasses, std::vector<Pass *> &computePasses);

#endif // GRAPHICS_H
