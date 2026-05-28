#include "graphics.h"
#include "mesh_loader.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

constexpr uint32_t QUEUE_INDEX = 0;
constexpr uint32_t WIDTH = 1920;
constexpr uint32_t HEIGHT = 969;
constexpr VkFormat FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D16_UNORM;
constexpr uint32_t SWAPCHAIN_SIZE = 3;
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

VkInstance g_instance;
VkPhysicalDevice g_physical_device;
VkDevice g_device;
VkQueue g_queue;
VkSurfaceKHR g_surface;
VkSwapchainKHR g_swapchain;
VkPipeline g_pipeline;
VkPipelineLayout g_pipeline_layout;
VkDescriptorSetLayout g_descriptor_layout;
VkSampler g_sampler;

glm::vec3 g_camera_position = glm::vec3(0., 0., 3.);
glm::mat4 g_model = glm::mat4(1.0f);
glm::mat4 g_view = glm::lookAtRH(g_camera_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
glm::mat4 g_projection = glm::perspectiveZO(glm::radians(60.0f),
                                            (float) WIDTH / (float) HEIGHT,
                                            0.1f,
                                            1000.0f);

UniformConstants g_constants = {g_model, g_view, g_projection, glm::vec4(1.)};

std::vector<VkImage> g_swapchain_images;
std::vector<VkImageView> g_swapchain_views;
Texture g_depth;

// Per frame data
std::vector<VkSemaphore> g_semaphores;
std::array<Frame, SWAPCHAIN_SIZE> g_frame_data{};

uint32_t find_memory_type(VkPhysicalDevice phy_device,
                          uint32_t filter_type,
                          VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(phy_device, &mem_properties);

    for (auto i = 0; i < mem_properties.memoryTypeCount; ++i) {
        if (filter_type & (1 << i)) {
            if ((mem_properties.memoryTypes[i].propertyFlags & props) == props)
                return i;
        }
    }

    throw;
}

VkShaderModule get_shader_module(const std::string path, VkShaderStageFlagBits bits)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw;

    auto byteSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (byteSize % sizeof(uint32_t) != 0)
        throw;

    std::vector<uint32_t> buffer(byteSize / sizeof(uint32_t));
    file.read(reinterpret_cast<char *>(buffer.data()), byteSize);

    VkShaderModuleCreateInfo create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    create_info.codeSize = buffer.size() * sizeof(uint32_t);
    create_info.pCode = buffer.data();

    VkShaderModule module;
    if (vkCreateShaderModule(g_device, &create_info, nullptr, &module) == VK_SUCCESS)
        std::cout << "Shader made" << std::endl;
    else
        std::cout << "Shader failed" << std::endl;
    return module;
}

void make_instance()
{
    uint32_t inst_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &inst_extension_count, nullptr);

    std::vector<VkExtensionProperties> extension{inst_extension_count};
    vkEnumerateInstanceExtensionProperties(nullptr, &inst_extension_count, extension.data());

    std::cout << "Found " << inst_extension_count << " extensions" << std::endl;

    VkApplicationInfo app_info{};
    app_info.pApplicationName = "Orbital";
    app_info.apiVersion = VK_API_VERSION_1_4;

    uint32_t ext_count = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&ext_count);

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app_info;
    info.enabledExtensionCount = ext_count;
    info.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&info, nullptr, &g_instance) == VK_SUCCESS)
        std::cout << "Created instance" << std::endl;
    else
        std::cout << "Creation instance failed" << std::endl;
}

void make_device()
{
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(g_instance, &gpu_count, nullptr);

    std::vector<VkPhysicalDevice> devices{gpu_count};
    vkEnumeratePhysicalDevices(g_instance, &gpu_count, devices.data());

    std::cout << "Found " << gpu_count << " gpus" << std::endl;

    g_physical_device = devices[0];

    // Assume everythings supported
    // Assume we pick queue 0

    float priority = 0.5;
    const char *extensions[] = {"VK_KHR_swapchain",
                                "VK_KHR_dynamic_rendering",
                                "VK_EXT_descriptor_buffer"};

    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .descriptorBuffer = VK_TRUE};

    VkPhysicalDeviceVulkan13Features enable_vulkan13_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &descriptor_features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = QUEUE_INDEX;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.pNext = &enable_vulkan13_features;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.ppEnabledExtensionNames = extensions;
    device_info.enabledExtensionCount = 3;

    if (vkCreateDevice(devices[0], &device_info, nullptr, &g_device) == VK_SUCCESS)
        std::cout << "Created device" << std::endl;
    else
        std::cout << "Creating device failed" << std::endl;

    vkGetDeviceQueue(g_device, QUEUE_INDEX, 0, &g_queue);
}

Buffer make_buffer(BufferDescription desc, void *data)
{
    Buffer result{};

    VkBufferCreateInfo vertex_b_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    vertex_b_info.size = desc.buffer_size;
    vertex_b_info.usage = desc.usage;
    vertex_b_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(g_device, &vertex_b_info, nullptr, &result.buffer) == VK_SUCCESS)
        std::cout << "Made triangle buffer" << std::endl;
    else
        std::cout << "Triangle buffer failed" << std::endl;

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(g_device, result.buffer, &mem_req);

    //Assume memory index 0
    VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = find_memory_type(g_physical_device,
                                                  mem_req.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(g_device, &alloc_info, nullptr, &result.memory) == VK_SUCCESS)
        std::cout << "Allocated triangle memory" << std::endl;
    else
        std::cout << "Triangle memory failed" << std::endl;

    vkBindBufferMemory(g_device, result.buffer, result.memory, 0);

    if (data != nullptr) {
        vkMapMemory(g_device, result.memory, 0, desc.buffer_size, 0, &result.mapped_data);
        memcpy(result.mapped_data, data, (size_t) desc.buffer_size);
        vkUnmapMemory(g_device, result.memory);
    }

    return result;
}

void init_per_frame(int index)
{
    VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(g_device, &info, nullptr, &g_frame_data[index].fence) == VK_SUCCESS)
        std::cout << "Fence made" << std::endl;
    else
        std::cout << "Fence failed" << std::endl;

    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = static_cast<uint32_t>(QUEUE_INDEX);

    if (vkCreateCommandPool(g_device, &pool_info, nullptr, &g_frame_data[index].pool) == VK_SUCCESS)
        std::cout << "pool made" << std::endl;
    else
        std::cout << "pool failed" << std::endl;

    VkCommandBufferAllocateInfo buf_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    buf_info.commandPool = g_frame_data[index].pool;
    buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buf_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(g_device, &buf_info, &g_frame_data[index].buffer) == VK_SUCCESS)
        std::cout << "buffer made" << std::endl;
    else
        std::cout << "buffer failed" << std::endl;
}

Texture make_image(TextureDescription desc, Image *image)
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

    vkCreateImage(g_device, &info, nullptr, &result.image);

    VkMemoryRequirements mem_reqs{};
    vkGetImageMemoryRequirements(g_device, result.image, &mem_reqs);

    VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = find_memory_type(g_physical_device,
                                                  mem_reqs.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(g_device, &alloc_info, nullptr, &result.memory);
    vkBindImageMemory(g_device, result.image, result.memory, 0);

    VkImageViewCreateInfo create_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    create_info.image = result.image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = desc.format;
    create_info.subresourceRange.aspectMask = desc.aspect;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    vkCreateImageView(g_device, &create_info, nullptr, &result.view);

    // Staging
    if (image != nullptr) {
        auto buffer = make_buffer({sizeof(unsigned char) * image->width * image->height
                                       * image->channels,
                                   VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT},
                                  image->data);

        // Create and update mip levels here, each level will need VkBufferImageCopy
        // https://docs.vulkan.org/samples/latest/samples/api/texture_mipmap_generation/README.html
        VkBufferImageCopy copy_region{};

        copy_region.imageSubresource.aspectMask = desc.aspect;
        copy_region.imageSubresource.mipLevel = 0;
        copy_region.imageSubresource.baseArrayLayer = 0;
        copy_region.imageSubresource.layerCount = 1;
        copy_region.imageExtent.width = image->width;
        copy_region.imageExtent.height = image->height;
        copy_region.imageExtent.depth = 1;
        copy_region.imageOffset = {0, 0, 0};
        copy_region.bufferOffset = {0};
        copy_region.bufferRowLength = 0;
        copy_region.bufferImageHeight = 0;

        // Pool for this, maybe need a higher order pool handler

        VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_info.queueFamilyIndex = static_cast<uint32_t>(QUEUE_INDEX);

        VkCommandPool pool;
        vkCreateCommandPool(g_device, &pool_info, nullptr, &pool);

        VkCommandBufferAllocateInfo buf_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        buf_info.commandPool = pool;
        buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        buf_info.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(g_device, &buf_info, &cmd);

        VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd, &begin_info);

        VkImageSubresourceRange range{};
        range.aspectMask = desc.aspect;
        range.baseMipLevel = 0;
        range.levelCount = 0;
        range.layerCount = 1;

        transition_image_layout(cmd,
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
                               &copy_region);

        transition_image_layout(cmd,
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
        VkSemaphore staging_semaphore;
        VkSemaphoreCreateInfo sem_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(g_device, &sem_info, nullptr, &staging_semaphore);

        VkFence staging_fence;
        VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(g_device, &fence_info, nullptr, &staging_fence);

        VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &staging_semaphore;

        vkQueueSubmit(g_queue, 1, &submit_info, staging_fence);

        vkWaitForFences(g_device, 1, &staging_fence, VK_TRUE, UINT64_MAX);
    }

    return result;
}

void make_swapchain()
{
    VkSurfaceCapabilitiesKHR surface_properties;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physical_device, g_surface, &surface_properties)
        == VK_SUCCESS)
        std::cout << "surface capabilities found" << std::endl;
    else
        std::cout << "surface capabilities not found" << std::endl;

    VkExtent2D swapchain_size;
    swapchain_size.width = WIDTH;
    swapchain_size.height = HEIGHT;

    uint32_t desired_images = SWAPCHAIN_SIZE; //Assuming more than min

    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = g_surface;
    info.minImageCount = desired_images;
    info.imageFormat = FORMAT; //Assuming we have this
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = swapchain_size;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.clipped = true;

    if (vkCreateSwapchainKHR(g_device, &info, nullptr, &g_swapchain) == VK_SUCCESS)
        std::cout << "Swapchain made" << std::endl;
    else
        std::cout << "Swapchain failed" << std::endl;

    uint32_t img_count;
    vkGetSwapchainImagesKHR(g_device, g_swapchain, &img_count, nullptr);
    g_swapchain_images.resize(img_count);
    g_swapchain_views.resize(img_count);
    vkGetSwapchainImagesKHR(g_device, g_swapchain, &img_count, g_swapchain_images.data());

    for (int i = 0; i < img_count; ++i)
        init_per_frame(i);

    for (auto i = 0; i < img_count; ++i) {
        VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.flags = 0;
        view_info.image = g_swapchain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = FORMAT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        vkCreateImageView(g_device, &view_info, nullptr, &g_swapchain_views[i]);
    }
}

void make_pipeline()
{
    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(UniformConstants);

    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &g_descriptor_layout;

    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &range;

    vkCreatePipelineLayout(g_device, &layout_info, nullptr, &g_pipeline_layout);

    VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(vertex);
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attr_description = {{
        {.location = 0,
         .binding = 0,
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = offsetof(vertex, position)},
        {.location = 1,
         .binding = 0,
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = offsetof(vertex, normal)},
        {.location = 2,
         .binding = 0,
         .format = VK_FORMAT_R32G32_SFLOAT,
         .offset = offsetof(vertex, uv)},
    }};

    VkPipelineVertexInputStateCreateInfo vertex_state_info{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_state_info.vertexBindingDescriptionCount = 1;
    vertex_state_info.vertexAttributeDescriptionCount = (uint32_t) attr_description.size();
    vertex_state_info.pVertexBindingDescriptions = &binding_desc;
    vertex_state_info.pVertexAttributeDescriptions = attr_description.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = false;

    VkPipelineRasterizationStateCreateInfo raster_info{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster_info.depthClampEnable = false;
    raster_info.rasterizerDiscardEnable = false;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.depthBiasEnable = false;
    raster_info.lineWidth = 1.0;

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR,
                                                  VK_DYNAMIC_STATE_CULL_MODE};

    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                      | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_state_info{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend_state_info.attachmentCount = 1;
    blend_state_info.pAttachments = &blend_attachment;

    VkPipelineViewportStateCreateInfo viewport_state{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state.depthTestEnable = VK_TRUE;
    depth_stencil_state.depthWriteEnable = VK_TRUE;

    VkPipelineMultisampleStateCreateInfo multisample_state{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDynamicStateCreateInfo dynamic_state_info{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
        {{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = get_shader_module(ROOT "shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
          .pName = "main"},
         {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = get_shader_module(ROOT "shaders/triangle.frag.spv",
                                      VK_SHADER_STAGE_FRAGMENT_BIT),
          .pName = "main"}}};

    VkPipelineRenderingCreateInfo rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &FORMAT;
    rendering_info.depthAttachmentFormat = DEPTH_FORMAT;

    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.pNext = &rendering_info;
    info.stageCount = static_cast<uint32_t>(shader_stages.size());
    info.pStages = shader_stages.data();
    info.pVertexInputState = &vertex_state_info;
    info.pInputAssemblyState = &input_assembly;
    info.pViewportState = &viewport_state;
    info.pRasterizationState = &raster_info;
    info.pMultisampleState = &multisample_state;
    info.pDepthStencilState = &depth_stencil_state;
    info.pColorBlendState = &blend_state_info;
    info.pDynamicState = &dynamic_state_info;
    info.layout = g_pipeline_layout;
    info.renderPass = VK_NULL_HANDLE;
    info.subpass = 0;

    if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &info, nullptr, &g_pipeline)
        == VK_SUCCESS)
        std::cout << "pipeline made" << std::endl;
    else
        std::cout << "pipeline failed" << std::endl;

    //delete shader modules
}

void make_descriptor()
{
    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = &g_sampler;

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.flags
        = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    info.bindingCount = 1;
    info.pBindings = &binding;

    vkCreateDescriptorSetLayout(g_device, &info, nullptr, &g_descriptor_layout);
}

void make_sampler()
{
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    vkCreateSampler(g_device, &info, nullptr, &g_sampler);
}

void transition_image_layout(VkCommandBuffer cmd,
                             VkImage img,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout,
                             VkImageAspectFlags aspectFlags,
                             VkAccessFlags2 srcAccessMask,
                             VkAccessFlags2 dstAccessMask,
                             VkPipelineStageFlags2 srcStage,
                             VkPipelineStageFlags2 dstStage)
{
    VkImageMemoryBarrier2 image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_barrier.srcStageMask = srcStage;
    image_barrier.dstStageMask = dstStage;
    image_barrier.srcAccessMask = srcAccessMask;
    image_barrier.dstAccessMask = dstAccessMask;
    image_barrier.oldLayout = oldLayout;
    image_barrier.newLayout = newLayout;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = img;
    image_barrier.subresourceRange.aspectMask = aspectFlags;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep_info.dependencyFlags = 0;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &image_barrier;

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

void render(uint32_t img, std::vector<NativeModel> models)
{
    static uint32_t frame = 0;

    auto cmd = g_frame_data[img].buffer;

    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);

    transition_image_layout(cmd,
                            g_swapchain_images[img],
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            0,
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    VkClearValue clear_color_value{}, depth_clear_value{};
    clear_color_value.color = {{0, 0, 0}};
    depth_clear_value.depthStencil = {1, 0};

    VkRenderingAttachmentInfo color_attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color_attachment.imageView = g_swapchain_views[img];
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue = clear_color_value;

    VkRenderingAttachmentInfo depth_attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth_attachment.imageView = g_depth.view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.clearValue = depth_clear_value;

    VkRenderingInfo rendering_info{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.renderArea.offset = {0, 0};
    rendering_info.renderArea.extent.width = WIDTH;
    rendering_info.renderArea.extent.height = HEIGHT;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    rendering_info.pDepthAttachment = &depth_attachment;

    vkCmdBeginRendering(cmd, &rendering_info);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);

    VkViewport vp{};
    vp.x = 0;
    vp.y = static_cast<float>(HEIGHT);
    vp.width = static_cast<float>(WIDTH);
    vp.height = -static_cast<float>(HEIGHT); // Flip viewport for Y up
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.extent.width = WIDTH;
    scissor.extent.height = HEIGHT;

    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdSetCullMode(cmd, VK_CULL_MODE_BACK_BIT);

    auto modelT = g_model;

    for (auto &model : models) {
        // updates go here

        g_view = glm::lookAt(g_camera_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        g_constants = {modelT,
                       g_view,
                       g_projection,
                       glm::vec4(g_camera_position.x, g_camera_position.y, g_camera_position.z, 0),
                       frame};
        //

        vkCmdPushConstants(cmd,
                           g_pipeline_layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(UniformConstants),
                           &g_constants);

        VkDeviceSize offset{0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &model.vertex.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, model.index.buffer, offset, VK_INDEX_TYPE_UINT16);

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = model.texture.view;
        image_info.sampler = VK_NULL_HANDLE; // Sampler is ummutable

        VkWriteDescriptorSet write_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write_set.dstBinding = 0;
        write_set.descriptorCount = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_set.pImageInfo = &image_info;

        vkCmdPushDescriptorSet(cmd,
                               VK_PIPELINE_BIND_POINT_GRAPHICS,
                               g_pipeline_layout,
                               0,
                               1,
                               &write_set);

        vkCmdDrawIndexed(cmd, model.index_count, 1, 0, 0, 0);

        modelT = glm::translate(modelT, glm::vec3(0, 0, 3));
    }
    vkCmdEndRendering(cmd);

    transition_image_layout(cmd,
                            g_swapchain_images[img],
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            0,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    vkEndCommandBuffer(cmd);

    if (g_frame_data[img].releaseSemaphore == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo sem_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(g_device, &sem_info, nullptr, &g_frame_data[img].releaseSemaphore);
    }

    VkPipelineStageFlags wait_stage{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT};

    VkSubmitInfo sub_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    sub_info.waitSemaphoreCount = 1;
    sub_info.pWaitSemaphores = &g_frame_data[img].acquireSemaphore;
    sub_info.pWaitDstStageMask = &wait_stage;
    sub_info.commandBufferCount = 1;
    sub_info.pCommandBuffers = &cmd;
    sub_info.signalSemaphoreCount = 1;
    sub_info.pSignalSemaphores = &g_frame_data[img].releaseSemaphore;

    vkQueueSubmit(g_queue, 1, &sub_info, g_frame_data[img].fence);

    frame++;
}

VkResult present_image(uint32_t index)
{
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &g_frame_data[index].releaseSemaphore;
    present.swapchainCount = 1;
    present.pSwapchains = &g_swapchain;
    present.pImageIndices = &index;

    return vkQueuePresentKHR(g_queue, &present);
}

VkResult acquire_swapchain_image(uint32_t *img)
{
    VkSemaphore semaphore;

    if (g_semaphores.empty()) {
        VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(g_device, &info, nullptr, &semaphore);
    } else {
        semaphore = g_semaphores.back();
        g_semaphores.pop_back();
    }

    auto res
        = vkAcquireNextImageKHR(g_device, g_swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, img);

    if (res != VK_SUCCESS) {
        g_semaphores.push_back(semaphore);
        return res;
    }

    if (g_frame_data[*img].fence != VK_NULL_HANDLE) {
        vkWaitForFences(g_device, 1, &g_frame_data[*img].fence, true, UINT64_MAX);
        vkResetFences(g_device, 1, &g_frame_data[*img].fence);
    }

    if (g_frame_data[*img].pool != VK_NULL_HANDLE) {
        vkResetCommandPool(g_device, g_frame_data[*img].pool, 0);
    }

    auto used_semaphore = g_frame_data[*img].acquireSemaphore;
    if (used_semaphore != VK_NULL_HANDLE)
        g_semaphores.push_back(used_semaphore);
    g_frame_data[*img].acquireSemaphore = semaphore;

    return res;
}

NativeModel make_native_model(Model &model)
{
    auto vbuffer = make_buffer({sizeof(vertex) * model.mesh.vertices.size(),
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
                               model.mesh.vertices.data());
    auto ibuffer = make_buffer({sizeof(uint16_t) * model.mesh.indices.size(),
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
                               model.mesh.indices.data());

    auto model_tex = make_image({model.material.textures[0].width,
                                 model.material.textures[0].height,
                                 VK_FORMAT_R8G8B8_SRGB,
                                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT},
                                &model.material.textures[0]);

    return {vbuffer, ibuffer, model_tex, static_cast<uint32_t>(model.mesh.indices.size())};
}

int g_main()
{
    if (!glfwInit())
        std::cout << "Issues!";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);

    auto *w = glfwCreateWindow(WIDTH, HEIGHT, "Orbital", NULL, NULL);

    if (!w)
        throw;

    make_instance();
    make_device();

    if (auto res = glfwCreateWindowSurface(g_instance, w, nullptr, &g_surface); res == VK_SUCCESS)
        std::cout << "Surface creation good" << std::endl;
    else
        std::cout << "Surface creation failed " << res << std::endl;

    make_swapchain();

    g_depth = make_image({WIDTH,
                          HEIGHT,
                          DEPTH_FORMAT,
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                          VK_IMAGE_ASPECT_DEPTH_BIT});

    make_sampler();
    make_descriptor();
    make_pipeline();

    glfwMakeContextCurrent(w);

    glfwShowWindow(w);

    struct WindowState
    {
        bool new_click;
        bool is_pressed;
    } state;

    glfwSetWindowUserPointer(w, &state);

    glfwSetMouseButtonCallback(w, [](GLFWwindow *window, int button, int action, int mods) {
        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                state->is_pressed = true;
                state->new_click = true;
            } else if (action == GLFW_RELEASE) {
                state->is_pressed = false;
            }
        }
    });

    glfwSetCursorPosCallback(w, [](GLFWwindow *window, double xpos, double ypos) {
        static double last_x_position = xpos, last_y_position = ypos;
        static double x_delta = 0.0;
        static double y_delta = 0.0;

        auto *state = static_cast<WindowState *>(glfwGetWindowUserPointer(window));

        if (!state->is_pressed)
            return;

        if (state->new_click) {
            last_x_position = xpos;
            last_y_position = ypos;
            state->new_click = false;
        }

        x_delta -= xpos - last_x_position;
        y_delta += ypos - last_y_position;
        y_delta = glm::clamp(y_delta, -130.0 + 0.01, 130.0 - 0.01);

        g_camera_position = glm::vec3(3. * (glm::sin(x_delta * 0.01) * glm::cos(y_delta * 0.01)),
                                      3. * (glm::sin(y_delta * 0.01)),
                                      3. * (glm::cos(x_delta * 0.01) * glm::cos(y_delta * 0.01)));

        last_x_position = xpos;
        last_y_position = ypos;
    });

    // ===== Load assets ======

    auto earth_asset = MeshLoader::load_model(ROOT "assets/earth.glb");
    auto monkey_asset = MeshLoader::load_model(ROOT "assets/monkey.glb");

    auto monkey = make_native_model(monkey_asset);
    auto earth = make_native_model(earth_asset);

    // ========================

    while (!glfwWindowShouldClose(w)) {
        uint32_t frame;
        auto r = acquire_swapchain_image(&frame);

        if (r != VK_SUCCESS) {
            vkQueueWaitIdle(g_queue);
            continue;
        }

        render(frame, {earth, monkey});
        present_image(frame);

        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    return 0;
}