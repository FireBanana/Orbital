#ifndef GRAPHICS_H
#define GRAPHICS_H

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include "asset_loader.h"

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
    TextureDescription description;
};

struct NativeModel
{
    Buffer vertex;
    Buffer index;
    std::unordered_map<TextureType, Texture> textures;
    uint32_t indexCount;
    vec3 position = {0, 0, 0};
    glm::mat4 worldTransform = glm::mat4(1.0);
};

class Graphics
{
public:
    Graphics(Window *window);

    Buffer makeBuffer(BufferDescription desc, void *data = nullptr);

    Texture makeImage(TextureDescription desc, Image *image = nullptr);

    // A Model is a scene from blender, so can have many individual objects
    std::vector<NativeModel> makeNativeModel(Model &model);

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
                         std::function<void(double time, double deltaTime)> updateFn);

    VkExtent2D getSwapchainSize() const;
    uint32_t getSwapchainCount() const;

private:
    uint32_t findMemoryType(VkPhysicalDevice phyDevice,
                            uint32_t filterType,
                            VkMemoryPropertyFlags props);

    void makeInstance();

    void makeDevice();

    void makeSwapchain();

    void recreateSwapchain(std::vector<Pass *> &graphicPasses);

    void makeRenderTarget(bool isRecreate);

    void initPerFrame(int index);

    void render(uint32_t img,
                std::vector<Pass *> graphicsPasses,
                std::vector<Pass *> computePasses);

    VkResult presentImage(uint32_t index);

    VkResult acquireSwapchainImage(uint32_t *img);

    Window *m_window;
    VkExtent2D m_swapchainSize{0, 0};
    uint32_t m_swapchainCount;
};

#endif // GRAPHICS_H
