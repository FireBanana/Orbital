#include "graphics.h"
#include "global.h"
#include "passes/pass.h"
#include "window.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

// TODO: Move compute stuff to a compute queue https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/performance/async_compute/README.adoc

uint32_t Graphics::findMemoryType(VkPhysicalDevice phyDevice,
                                  uint32_t filterType,
                                  VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(phyDevice, &memProperties);

    for (auto i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (filterType & (1 << i)) {
            if ((memProperties.memoryTypes[i].propertyFlags & props) == props)
                return i;
        }
    }

    throw;
}

VkShaderModule Graphics::getShaderModule(const std::string path, VkShaderStageFlagBits bits)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        printf("File open failed\n");
        std::cerr << strerror(errno) << std::endl;
        throw;
    }

    auto byteSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (byteSize % sizeof(uint32_t) != 0)
        throw;

    std::vector<uint32_t> buffer(byteSize / sizeof(uint32_t));
    file.read(reinterpret_cast<char *>(buffer.data()), byteSize);

    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule module;
    if (vkCreateShaderModule(Global::g_device, &createInfo, nullptr, &module) == VK_SUCCESS)
        std::cout << "Shader made" << std::endl;
    else
        std::cout << "Shader failed" << std::endl;
    return module;
}

void Graphics::makeInstance()
{
    uint32_t instExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &instExtensionCount, nullptr);

    std::vector<VkExtensionProperties> extension{instExtensionCount};
    vkEnumerateInstanceExtensionProperties(nullptr, &instExtensionCount, extension.data());

    std::cout << "Found " << instExtensionCount << " extensions" << std::endl;

    VkApplicationInfo appInfo{};
    appInfo.pApplicationName = "Orbital";
    appInfo.apiVersion = VK_API_VERSION_1_4;

    uint32_t extCount = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&extCount);

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &appInfo;
    info.enabledExtensionCount = extCount;
    info.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&info, nullptr, &Global::g_instance) == VK_SUCCESS)
        std::cout << "Created instance" << std::endl;
    else
        std::cout << "Creation instance failed" << std::endl;
}

void Graphics::makeDevice()
{
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(Global::g_instance, &gpuCount, nullptr);

    std::vector<VkPhysicalDevice> devices{gpuCount};
    vkEnumeratePhysicalDevices(Global::g_instance, &gpuCount, devices.data());

    std::cout << "Found " << gpuCount << " gpus" << std::endl;

    Global::g_physical_device = devices[0];

    // Assume everythings supported
    // Assume we pick queue 0

    float priority = 0.5;
    const char *extensions[] = {"VK_KHR_swapchain"};

    VkPhysicalDeviceVulkan14Features
        enableVulkan14Features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
                                 .pushDescriptor = VK_TRUE};

    VkPhysicalDeviceVulkan11Features enableVulkan11Features
        = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
           .pNext = &enableVulkan14Features,
           .shaderDrawParameters = VK_TRUE};

    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .pNext = &enableVulkan11Features,
        .descriptorBuffer = VK_TRUE};

    VkPhysicalDeviceVulkan13Features enableVulkan13Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &descriptorFeatures,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = Global::QUEUE_INDEX;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.pNext = &enableVulkan13Features;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.ppEnabledExtensionNames = extensions;
    deviceInfo.enabledExtensionCount = 1;

    if (vkCreateDevice(devices[0], &deviceInfo, nullptr, &Global::g_device) == VK_SUCCESS)
        std::cout << "Created device" << std::endl;
    else
        std::cout << "Creating device failed" << std::endl;

    vkGetDeviceQueue(Global::g_device, Global::QUEUE_INDEX, 0, &Global::g_queue);
}

Graphics::Graphics(Window *window)
    : m_window(window)
{
    makeInstance();
    makeDevice();
    window->makeSurface();
    makeSwapchain();
}

Buffer Graphics::makeBuffer(BufferDescription desc, void *data)
{
    Buffer result{};

    VkBufferCreateInfo vertexBInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    vertexBInfo.size = desc.bufferSize;
    vertexBInfo.usage = desc.usage;
    vertexBInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Global::g_device, &vertexBInfo, nullptr, &result.buffer) == VK_SUCCESS)
        std::cout << "Made triangle buffer" << std::endl;
    else
        std::cout << "Triangle buffer failed" << std::endl;

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(Global::g_device, result.buffer, &memReq);

    //Assume memory index 0
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(Global::g_physical_device, // Add cached bit option
                                                  memReq.memoryTypeBits,
                                                  desc.memProperty);
    if (vkAllocateMemory(Global::g_device, &allocInfo, nullptr, &result.memory) == VK_SUCCESS)
        std::cout << "Allocated triangle memory" << std::endl;
    else
        std::cout << "Triangle memory failed" << std::endl;

    vkBindBufferMemory(Global::g_device, result.buffer, result.memory, 0);

    // Should also handle cases where its both host and local, or at least output error
    if (data != nullptr && desc.memProperty & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        auto buffer = makeBuffer({desc.bufferSize,
                                  VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
                                 data);

        VkBufferCopy copyRegion{};
        copyRegion.size = desc.bufferSize;

        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = static_cast<uint32_t>(Global::QUEUE_INDEX);

        VkCommandPool pool;
        vkCreateCommandPool(Global::g_device, &poolInfo, nullptr, &pool);

        VkCommandBufferAllocateInfo bufInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        bufInfo.commandPool = pool;
        bufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bufInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(Global::g_device, &bufInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd, &beginInfo);

        transitionBuffer(cmd,
                         result.buffer,
                         {},
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        vkCmdCopyBuffer(cmd, buffer.buffer, result.buffer, 1, &copyRegion);

        transitionBuffer(cmd,
                         result.buffer,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_ACCESS_2_SHADER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkEndCommandBuffer(cmd);

        VkFence stagingFence;
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(Global::g_device, &fenceInfo, nullptr, &stagingFence);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        submitInfo.signalSemaphoreCount = 0;
        vkQueueSubmit(Global::g_queue, 1, &submitInfo, stagingFence);

        vkWaitForFences(Global::g_device, 1, &stagingFence, VK_TRUE, UINT64_MAX);
    } else if (data == nullptr
               && desc.memProperty
                      & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                         | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        vkMapMemory(Global::g_device, result.memory, 0, desc.bufferSize, 0, &result.mappedData);
        // Needs to be unmapped at some point?
    } else if (data != nullptr) {
        vkMapMemory(Global::g_device, result.memory, 0, desc.bufferSize, 0, &result.mappedData);
        memcpy(result.mappedData, data, (size_t) desc.bufferSize);
        vkUnmapMemory(Global::g_device, result.memory);
    }

    return result;
}

void Graphics::initPerFrame(int index)
{
    VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(Global::g_device, &info, nullptr, &Global::g_frame_data[index].fence)
        == VK_SUCCESS)
        std::cout << "Fence made" << std::endl;
    else
        std::cout << "Fence failed" << std::endl;

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(Global::QUEUE_INDEX);

    if (vkCreateCommandPool(Global::g_device, &poolInfo, nullptr, &Global::g_frame_data[index].pool)
        == VK_SUCCESS)
        std::cout << "pool made" << std::endl;
    else
        std::cout << "pool failed" << std::endl;

    VkCommandBufferAllocateInfo bufInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    bufInfo.commandPool = Global::g_frame_data[index].pool;
    bufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(Global::g_device, &bufInfo, &Global::g_frame_data[index].buffer)
        == VK_SUCCESS)
        std::cout << "buffer made" << std::endl;
    else
        std::cout << "buffer failed" << std::endl;
}

Texture Graphics::makeImage(TextureDescription desc, Image *image)
{
    Texture result{};

    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.format = desc.format;
    info.usage = desc.usage;
    info.extent.width = desc.width;
    info.extent.height = desc.height;
    info.extent.depth = 1;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;

    vkCreateImage(Global::g_device, &info, nullptr, &result.image);

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(Global::g_device, result.image, &memReqs);

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(Global::g_physical_device,
                                                memReqs.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(Global::g_device, &allocInfo, nullptr, &result.memory);
    vkBindImageMemory(Global::g_device, result.image, result.memory, 0);

    VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.image = result.image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = desc.format;
    createInfo.subresourceRange.aspectMask = desc.aspect;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(Global::g_device, &createInfo, nullptr, &result.view);

    // Staging
    if (image != nullptr) {
        auto buffer = makeBuffer({sizeof(unsigned char) * image->width * image->height
                                       * image->channels,
                                   VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                       | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
                                  image->data);

        // Create and update mip levels here, each level will need VkBufferImageCopy
        // https://docs.vulkan.org/samples/latest/samples/api/texture_mipmap_generation/README.html
        VkBufferImageCopy copyRegion{};

        copyRegion.imageSubresource.aspectMask = desc.aspect;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent.width = image->width;
        copyRegion.imageExtent.height = image->height;
        copyRegion.imageExtent.depth = 1;
        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.bufferOffset = {0};
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        // Pool for this, maybe need a higher order pool handler

        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = static_cast<uint32_t>(Global::QUEUE_INDEX);

        VkCommandPool pool;
        vkCreateCommandPool(Global::g_device, &poolInfo, nullptr, &pool);

        VkCommandBufferAllocateInfo bufInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        bufInfo.commandPool = pool;
        bufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bufInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(Global::g_device, &bufInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageSubresourceRange range{};
        range.aspectMask = desc.aspect;
        range.baseMipLevel = 0;
        range.levelCount = 0;
        range.layerCount = 1;

        transitionImageLayout(cmd,
                              result.image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              desc.aspect,
                              {},
                              VK_ACCESS_2_TRANSFER_WRITE_BIT,
                              VK_PIPELINE_STAGE_2_HOST_BIT,
                              VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        vkCmdCopyBufferToImage(cmd,
                               buffer.buffer,
                               result.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &copyRegion);

        transitionImageLayout(cmd,
                              result.image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              desc.aspect,
                              VK_ACCESS_2_TRANSFER_WRITE_BIT,
                              VK_ACCESS_2_SHADER_READ_BIT,
                              VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        vkEndCommandBuffer(cmd);

        //submit
        VkSemaphore stagingSemaphore;
        VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(Global::g_device, &semInfo, nullptr, &stagingSemaphore);

        VkFence stagingFence;
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(Global::g_device, &fenceInfo, nullptr, &stagingFence);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &stagingSemaphore;

        vkQueueSubmit(Global::g_queue, 1, &submitInfo, stagingFence);

        vkWaitForFences(Global::g_device, 1, &stagingFence, VK_TRUE, UINT64_MAX);
    }

    result.isValid = true;
    result.description = desc;
    return result;
}

void Graphics::makeRenderTarget(bool isRecreate)
{
    if (isRecreate) {
        for (auto i = 0; i < m_swapchainCount; ++i) {
            vkDestroyImage(Global::g_device, Global::g_render_targets[i].image, nullptr);
            vkDestroyImageView(Global::g_device, Global::g_render_targets[i].view, nullptr);
            vkFreeMemory(Global::g_device, Global::g_render_targets[i].memory, nullptr);
        }

        Global::g_render_targets.clear();
    } else if (Global::g_render_targets.size() != 0) {
        std::cout << "Error, swapchain images being created without swapchain recreation!";
    }

    for (auto i = 0; i < m_swapchainCount; ++i) {
        auto target = makeImage({m_swapchainSize.width,
                                 m_swapchainSize.height,
                                 Global::RENDER_TARGET_FORMAT,
                                 VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                     | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT});

        Global::g_render_targets.push_back(std::move(target));
    }
}

void Graphics::makeSwapchain()
{
    auto oldSwapchain = Global::g_swapchain;
    bool isRecreate = oldSwapchain != VK_NULL_HANDLE;

    if (isRecreate)
        vkDeviceWaitIdle(Global::g_device);

    VkSurfaceCapabilitiesKHR surfaceProperties;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Global::g_physical_device,
                                                  Global::g_surface,
                                                  &surfaceProperties)
        == VK_SUCCESS)
        std::cout << "surface capabilities found" << std::endl;
    else
        std::cout << "surface capabilities not found" << std::endl;

    if (surfaceProperties.currentExtent.width == 0xFFFFFFFF) {
        m_swapchainSize = m_window->getExtent();
    } else {
        m_swapchainSize.width = surfaceProperties.currentExtent.width;
        m_swapchainSize.height = surfaceProperties.currentExtent.height;
    }

    m_swapchainCount = surfaceProperties.minImageCount;
    Global::g_frame_data.resize(m_swapchainCount);

    makeRenderTarget(isRecreate);

    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = Global::g_surface;
    info.minImageCount = m_swapchainCount;
    info.imageFormat = Global::FORMAT; //Assuming we have this
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = m_swapchainSize;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform
        = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // Not optimal on devices that support rotation
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    info.clipped = true;
    info.oldSwapchain = oldSwapchain;

    if (vkCreateSwapchainKHR(Global::g_device, &info, nullptr, &Global::g_swapchain) == VK_SUCCESS)
        std::cout << "Swapchain made" << std::endl;
    else
        std::cout << "Swapchain failed" << std::endl;

    uint32_t imgCount;
    vkGetSwapchainImagesKHR(Global::g_device, Global::g_swapchain, &imgCount, nullptr);
    Global::g_swapchain_images.resize(imgCount);
    Global::g_swapchain_views.resize(imgCount);
    vkGetSwapchainImagesKHR(Global::g_device,
                            Global::g_swapchain,
                            &imgCount,
                            Global::g_swapchain_images.data());

    if (!isRecreate)
        for (int i = 0; i < imgCount; ++i)
            initPerFrame(i);

    for (auto i = 0; i < imgCount; ++i) {
        if (isRecreate)
            vkDestroyImageView(Global::g_device, Global::g_swapchain_views[i], nullptr);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.flags = 0;
        viewInfo.image = Global::g_swapchain_images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = Global::FORMAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(Global::g_device, &viewInfo, nullptr, &Global::g_swapchain_views[i]);
    }

    if (oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(Global::g_device, oldSwapchain, nullptr);
}

void Graphics::recreateSwapchain(std::vector<Pass *> &graphicPasses)
{
    //cleanup
    Global::g_swapchain_dirty = true;
    makeSwapchain();

    for(auto p : graphicPasses) {
        if (!p->m_isUsingEngineTargets) {
            // Resize color targets here
        }

        if (p->m_depth != nullptr) {
            vkDestroyImageView(Global::g_device, p->m_depth->view, nullptr);
            vkDestroyImage(Global::g_device, p->m_depth->image, nullptr);
            vkFreeMemory(Global::g_device, p->m_depth->memory, nullptr);

            auto newDesc = p->m_depth->description;
            newDesc.width = m_swapchainSize.width;
            newDesc.height = m_swapchainSize.height;

            *p->m_depth = makeImage(newDesc);
        }
    }

    Global::g_swapchain_dirty = false;
}

void Graphics::transitionImageLayout(VkCommandBuffer cmd,
                           VkImage img,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout,
                             VkImageAspectFlags aspectFlags,
                             VkAccessFlags2 srcAccessMask,
                             VkAccessFlags2 dstAccessMask,
                             VkPipelineStageFlags2 srcStage,
                             VkPipelineStageFlags2 dstStage)
{
    VkImageMemoryBarrier2 imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    imageBarrier.srcStageMask = srcStage;
    imageBarrier.dstStageMask = dstStage;
    imageBarrier.srcAccessMask = srcAccessMask;
    imageBarrier.dstAccessMask = dstAccessMask;
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = img;
    imageBarrier.subresourceRange.aspectMask = aspectFlags;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo depInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    depInfo.dependencyFlags = 0;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void Graphics::transitionBuffer(VkCommandBuffer cmd,
                      VkBuffer buffer,
                      VkAccessFlags2 srcAccess,
                      VkAccessFlags2 dstAccess,
                      VkPipelineStageFlags2 srcStage,
                      VkPipelineStageFlags2 dstStage)
{
    VkBufferMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.buffer = buffer;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    VkDependencyInfo depInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    depInfo.dependencyFlags = 0;
    depInfo.bufferMemoryBarrierCount = 1;
    depInfo.pBufferMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

// Texture target for each swapchain image and 1 depth texture by default
void Graphics::render(uint32_t img,
                      std::vector<Pass *> graphicsPasses,
                      std::vector<Pass *> computePasses)
{
    auto cmd = Global::g_frame_data[img].buffer;

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);

    // Transition targets to color attachment
    transitionImageLayout(cmd,
                          Global::g_render_targets[img].image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          0,
                          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
                              | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
                          VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    // Render target clear pass ====
    VkClearValue clearValue{};
    clearValue.color = {{0, 0, 0, 0}};

    VkRenderingAttachmentInfo clearAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    clearAttachment.imageView = Global::g_render_targets[img].view;
    clearAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    clearAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    clearAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    clearAttachment.clearValue = clearValue;

    VkRenderingInfo clearInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
    clearInfo.renderArea = {{0, 0}, m_swapchainSize};
    clearInfo.layerCount = 1;
    clearInfo.colorAttachmentCount = 1;
    clearInfo.pColorAttachments = &clearAttachment;

    vkCmdBeginRendering(cmd, &clearInfo);
    vkCmdEndRendering(cmd);
    //================================

    // Graphic Passes
    for (auto &pass : graphicsPasses)
        pass->render(&cmd, img);

    // Transition swapchain to storage
    transitionImageLayout(cmd,
                          Global::g_render_targets[img].image,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                          VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                          VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    // Compute Passes
    std::vector<Texture> res{{.view = Global::g_render_targets[img].view}};
    for (auto &pass : computePasses) {
        pass->attachImageResources(&res);
        pass->render(&cmd, img);
    }

    transitionImageLayout(cmd,
                          Global::g_render_targets[img].image,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_ACCESS_2_SHADER_WRITE_BIT,
                          VK_ACCESS_2_TRANSFER_READ_BIT,
                          VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

    blitRegion.srcOffsets[1].x = m_swapchainSize.width;
    blitRegion.srcOffsets[1].y = m_swapchainSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = m_swapchainSize.width;
    blitRegion.dstOffsets[1].y = m_swapchainSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
    blitInfo.srcImage = Global::g_render_targets[img].image;
    blitInfo.dstImage = Global::g_swapchain_images[img];
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    transitionImageLayout(cmd,
                          Global::g_swapchain_images[img],
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          {},
                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    vkCmdBlitImage2(cmd, &blitInfo);

    transitionImageLayout(cmd,
                          Global::g_swapchain_images[img],
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    vkEndCommandBuffer(cmd);

    if (Global::g_frame_data[img].releaseSemaphore == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(Global::g_device,
                          &semInfo,
                          nullptr,
                          &Global::g_frame_data[img].releaseSemaphore);
    }

    VkPipelineStageFlags waitStage{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT};

    VkSubmitInfo subInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    subInfo.waitSemaphoreCount = 1;
    subInfo.pWaitSemaphores = &Global::g_frame_data[img].acquireSemaphore;
    subInfo.pWaitDstStageMask = &waitStage;
    subInfo.commandBufferCount = 1;
    subInfo.pCommandBuffers = &cmd;
    subInfo.signalSemaphoreCount = 1;
    subInfo.pSignalSemaphores = &Global::g_frame_data[img].releaseSemaphore;

    vkQueueSubmit(Global::g_queue, 1, &subInfo, Global::g_frame_data[img].fence);

    // Readback data
    for (auto &pass : computePasses) {
        pass->read(img);
    }
}

VkResult Graphics::presentImage(uint32_t index)
{
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &Global::g_frame_data[index].releaseSemaphore;
    present.swapchainCount = 1;
    present.pSwapchains = &Global::g_swapchain;
    present.pImageIndices = &index;

    return vkQueuePresentKHR(Global::g_queue, &present);
}

VkResult Graphics::acquireSwapchainImage(uint32_t *img)
{
    VkSemaphore semaphore;

    if (Global::g_semaphores.empty()) {
        VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(Global::g_device, &info, nullptr, &semaphore);
    } else {
        semaphore = Global::g_semaphores.back();
        Global::g_semaphores.pop_back();
    }

    auto res = vkAcquireNextImageKHR(Global::g_device,
                                     Global::g_swapchain,
                                     UINT64_MAX,
                                     semaphore,
                                     VK_NULL_HANDLE,
                                     img);

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        Global::g_semaphores.push_back(semaphore);
        return res;
    }

    if (Global::g_frame_data[*img].fence != VK_NULL_HANDLE) {
        vkWaitForFences(Global::g_device, 1, &Global::g_frame_data[*img].fence, true, UINT64_MAX);
        vkResetFences(Global::g_device, 1, &Global::g_frame_data[*img].fence);
    }

    if (Global::g_frame_data[*img].pool != VK_NULL_HANDLE) {
        vkResetCommandPool(Global::g_device, Global::g_frame_data[*img].pool, 0);
    }

    auto usedSemaphore = Global::g_frame_data[*img].acquireSemaphore;
    if (usedSemaphore != VK_NULL_HANDLE)
        Global::g_semaphores.push_back(usedSemaphore);
    Global::g_frame_data[*img].acquireSemaphore = semaphore;

    return res;
}

std::vector<NativeModel> Graphics::makeNativeModel(Model &model)
{
    std::vector<NativeModel> result;

    for (auto &m : model.meshes) {
        auto vbuffer = makeBuffer({sizeof(vertex) * m.vertices.size(),
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                                       | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
                                  m.vertices.data());
        auto ibuffer = makeBuffer({sizeof(uint32_t) * m.indices.size(),
                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                                       | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
                                  m.indices.data());

        Texture modelTex{};

        if (m.textureIndex != -1) {
            modelTex = makeImage({model.textures[m.textureIndex].width,
                                  model.textures[m.textureIndex].height,
                                  VK_FORMAT_R8G8B8A8_SRGB,
                                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT},
                                 &model.textures[m.textureIndex]);
        }

        NativeModel nm{vbuffer, ibuffer, modelTex, static_cast<uint32_t>(m.indices.size())};
        nm.worldTransform = m.worldTransform;
        result.push_back(std::move(nm));
    }

    return result;
}

void Graphics::beginRenderLoop(std::vector<Pass *> &graphicsPasses,
                               std::vector<Pass *> &computePasses,
                               std::function<void(double time, double deltaTime)> updateFn)
{
    Global::g_gui_thread = std::thread([&]() {
        while (!glfwWindowShouldClose(Global::g_window)) {
            if (Global::g_swapchain_dirty)
                recreateSwapchain(graphicsPasses);

            uint32_t frame;

            auto r = acquireSwapchainImage(&frame);

            if (r == VK_ERROR_OUT_OF_DATE_KHR) {
                //vkQueueWaitIdle(Global::g_queue);
                recreateSwapchain(graphicsPasses);
                continue;
            }

            render(frame, graphicsPasses, computePasses);
            r = presentImage(frame);

            if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR)
                recreateSwapchain(graphicsPasses);

            glfwPollEvents();
        }

        Global::g_window_running = false;
    });

    uint64_t frame = 0;

    double t = 0.0;
    double dt = 1000 / 60.0;
    auto currTime = std::chrono::high_resolution_clock::now();

    // Main loop
    while (1) {
        if (!Global::g_window_running)
            break;

        auto newTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<double, std::milli>(newTime - currTime).count();
        currTime = newTime;

        while (frameTime > 0.0) {
            float delta = std::min(frameTime, dt);

            //processing
            updateFn(t, delta);

            frameTime -= delta;
            t += delta;
        }

        frame++;
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(dt));
    }

    Global::g_gui_thread.join();
}

VkExtent2D Graphics::getSwapchainSize() const
{
    return m_swapchainSize;
}

uint32_t Graphics::getSwapchainCount() const
{
    return m_swapchainCount;
}
