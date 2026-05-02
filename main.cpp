#include <cstring>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

constexpr uint32_t QUEUE_INDEX = 0;
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr VkFormat FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
constexpr uint32_t SWAPCHAIN_SIZE = 3;
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

VkInstance g_instance;
VkPhysicalDevice g_physical_device;
VkDevice g_device;
VkQueue g_queue;
VkSurfaceKHR g_surface;
VkSwapchainKHR g_swapchain;
VkPipeline g_pipeline;

// Per frame data
std::array<VkSemaphore, FRAMES_IN_FLIGHT> g_acquire_semaphores;
std::array<VkSemaphore, FRAMES_IN_FLIGHT> g_present_semaphores;
std::array<VkFence, FRAMES_IN_FLIGHT> g_fences;
std::array<VkCommandPool, FRAMES_IN_FLIGHT> g_command_pool;

VkBuffer g_triangle_buffer;

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;
};

// Triangle buffer
const std::vector<Vertex> vertices = {
    {{0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Vertex 1: Red
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  // Vertex 2: Green
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}  // Vertex 3: Blue
};

uint32_t find_memory_type(
    VkPhysicalDevice phy_device, uint32_t filter_type, VkMemoryPropertyFlags props)
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
    // uint32_t inst_extension_count = 0;
    // vkEnumerateInstanceExtensionProperties(nullptr, &inst_extension_count, nullptr);

    // std::vector<VkExtensionProperties> extension{inst_extension_count};
    // vkEnumerateInstanceExtensionProperties(nullptr, &inst_extension_count, extension.data());

    // std::cout << "Found " << inst_extension_count << " extensions" <<std::endl;

    VkApplicationInfo app_info{};
    app_info.pApplicationName = "Orbital";
    app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 0);

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
    const char *extensions[] = {"VK_KHR_swapchain"};

    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = QUEUE_INDEX;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.ppEnabledExtensionNames = extensions;
    device_info.enabledExtensionCount = 1;

    if (vkCreateDevice(devices[0], &device_info, nullptr, &g_device) == VK_SUCCESS)
        std::cout << "Created device" << std::endl;
    else
        std::cout << "Creating device failed" << std::endl;

    vkGetDeviceQueue(g_device, QUEUE_INDEX, 0, &g_queue);
}

void make_vertex_buffer()
{
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    VkBufferCreateInfo vertex_b_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    vertex_b_info.size = buffer_size;
    vertex_b_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertex_b_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(g_device, &vertex_b_info, nullptr, &g_triangle_buffer) == VK_SUCCESS)
        std::cout << "Made triangle buffer" << std::endl;
    else
        std::cout << "Triangle buffer failed" << std::endl;

    VkMemoryRequirements mem_req;
    VkDeviceMemory buff_mem;
    vkGetBufferMemoryRequirements(g_device, g_triangle_buffer, &mem_req);

    //Assume memory index 0
    VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = find_memory_type(
        g_physical_device,
        mem_req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(g_device, &alloc_info, nullptr, &buff_mem) == VK_SUCCESS)
        std::cout << "Allocated triangle memory" << std::endl;
    else
        std::cout << "Triangle memory failed" << std::endl;

    vkBindBufferMemory(g_device, g_triangle_buffer, buff_mem, 0);

    void *data;
    vkMapMemory(g_device, buff_mem, 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), (size_t) buffer_size);
    vkUnmapMemory(g_device, buff_mem);
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
}

void make_pipeline()
{
    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VkPipelineLayout layout;

    vkCreatePipelineLayout(g_device, &layout_info, nullptr, &layout);

    VkVertexInputBindingDescription binding_desc{};
    binding_desc.binding = 0;
    binding_desc.stride = sizeof(Vertex);
    binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attr_description = {{
        {.location = 0,
         .binding = 0,
         .format = VK_FORMAT_R32G32_SFLOAT,
         .offset = offsetof(Vertex, position)},
        {.location = 1,
         .binding = 0,
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = offsetof(Vertex, color)},
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
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_ALWAYS;

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
          .module = get_shader_module(ROOT "shaders/triangle.vert.spirv",
                                      VK_SHADER_STAGE_VERTEX_BIT),
          .pName = "main"},
         {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = get_shader_module(ROOT "shaders/triangle.frag.spirv",
                                      VK_SHADER_STAGE_FRAGMENT_BIT),
          .pName = "main"}}};

    VkPipelineRenderingCreateInfo rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &FORMAT;

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
    info.layout = layout;
    info.renderPass = VK_NULL_HANDLE;
    info.subpass = 0;

    if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &info, nullptr, &g_pipeline)
        == VK_SUCCESS)
        std::cout << "pipeline made" << std::endl;
    else
        std::cout << "pipeline failed" << std::endl;

    //delete shader modules
}

void init_per_frame();

void render_triangle(uint32_t swapchain_index) {}

VkResult present_image(uint32_t index) {}

VkResult acquire_swapchain_image(uint32_t *img)
{
    static uint32_t current_frame = 0;

    auto res = vkAcquireNextImageKHR(g_device,
                                     g_swapchain,
                                     UINT64_MAX,
                                     g_acquire_semaphores[current_frame],
                                     VK_NULL_HANDLE,
                                     img);

    if (res != VK_SUCCESS)
        return res;

    if (g_fences[*img] != VK_NULL_HANDLE) {
        vkWaitForFences(g_device, 1, &g_fences[*img], true, UINT64_MAX);
        vkResetFences(g_device, 1, &g_fences[*img]);
    }

    if (g_command_pool[*img] != VK_NULL_HANDLE) {
        vkResetCommandPool(g_device, g_command_pool[*img], 0);
    }

    current_frame = (current_frame + 1) % FRAMES_IN_FLIGHT;

    return res;
}

int main()
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
    make_vertex_buffer();

    if (auto res = glfwCreateWindowSurface(g_instance, w, nullptr, &g_surface); res == VK_SUCCESS)
        std::cout << "Surface creation good" << std::endl;
    else
        std::cout << "Surface creation failed " << res << std::endl;

    make_swapchain();
    make_pipeline();
    init_per_frame();

    glfwMakeContextCurrent(w);

    glfwShowWindow(w);

    while (!glfwWindowShouldClose(w)) {
        uint32_t frame;
        auto r = acquire_swapchain_image(&frame);

        if (r != VK_SUCCESS) {
            vkQueueWaitIdle(g_queue);
            continue;
        }

        render_triangle(frame);
        r = present_image(frame);

        glfwPollEvents();
    }
}
