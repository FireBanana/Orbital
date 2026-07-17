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
class Window;

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
    VkDeviceSize bufferSize;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memProperty;
};

struct Buffer
{
    VkDeviceMemory memory;
    VkBuffer buffer;
    void *mappedData;
};

struct Texture
{
    bool isValid;
    VkDeviceMemory memory;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkExtent2D extent;
    uint32_t mipLevels;
};

struct NativeModel
{
    Buffer vertex;
    Buffer index;
    Texture texture;
    uint32_t indexCount;
    vec3 position = {0, 0, 0};
};

class Graphics
{
public:
    Graphics(Window *window);

    Buffer makeBuffer(BufferDescription desc, void *data = nullptr);

    Texture makeImage(TextureDescription desc, Image *image = nullptr);

    NativeModel makeNativeModel(Model &model);

    VkShaderModule getShaderModule(const std::string path, VkShaderStageFlagBits bits);

    void transitionImageLayout(VkCommandBuffer cmd,
                               VkImage img,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               VkImageAspectFlags flags,
                               VkAccessFlags2 srcAccessMask,
                               VkAccessFlags2 dstAccessMask,
                               VkPipelineStageFlags2 srcStage,
                               VkPipelineStageFlags2 dstStage);

    void transitionBuffer(VkCommandBuffer cmd,
                          VkBuffer buffer,
                          VkAccessFlags2 srcAccess,
                          VkAccessFlags2 dstAccess,
                          VkPipelineStageFlags2 srcStage,
                          VkPipelineStageFlags2 dstStage);

    void beginRenderLoop(std::vector<Pass *> &graphicsPasses,
                         std::vector<Pass *> &computePasses,
                         std::function<void(double)> updateFn);

    VkExtent2D getSwapchainSize() const;

private:
    uint32_t findMemoryType(VkPhysicalDevice phyDevice,
                            uint32_t filterType,
                            VkMemoryPropertyFlags props);

    void makeInstance();

    void makeDevice();

    void makeSwapchain();

    void recreateSwapchain();

    void makeRenderTarget();

    void initPerFrame(int index);

    void render(uint32_t img,
                std::vector<Pass *> graphicsPasses,
                std::vector<Pass *> computePasses);

    VkResult presentImage(uint32_t index);

    VkResult acquireSwapchainImage(uint32_t *img);

    Window *m_window;
};

#endif // GRAPHICS_H
