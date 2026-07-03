#include "n_body_compute.h"
#include "../global.h"

GravityComputePass::GravityComputePass()
{
    create_descriptor();
    create_pipeline();
    generateData();
}

void GravityComputePass::render(VkCommandBuffer *cmd)
{
    vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView = m_textures->at(0).view;
    image_info.sampler = VK_NULL_HANDLE; // Sampler is ummutable

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = m_dataBuffer.buffer;
    buffer_info.range = m_bodies.size() * sizeof(glm::vec4);

    VkWriteDescriptorSet image_write_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    image_write_set.dstBinding = 0;
    image_write_set.descriptorCount = 1;
    image_write_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    image_write_set.pImageInfo = &image_info;

    VkWriteDescriptorSet buffer_write_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    buffer_write_set.dstBinding = 1;
    buffer_write_set.descriptorCount = 1;
    buffer_write_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    buffer_write_set.pBufferInfo = &buffer_info;

    std::array<VkWriteDescriptorSet, 2> writeSets{image_write_set, buffer_write_set};

    vkCmdPushDescriptorSet(*cmd,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           m_pipelineLayout,
                           0,
                           2,
                           writeSets.data());

    vkCmdDispatch(*cmd, m_bodies.size(), m_bodies.size(), 1);
}

void GravityComputePass::create_pipeline()
{
    // VkPushConstantRange range{};
    // range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    // range.offset = 0;
    // range.size = sizeof(UniformConstants);

    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &m_descriptorSetLayout;

    // layout_info.pushConstantRangeCount = 1;
    // layout_info.pPushConstantRanges = &range;

    vkCreatePipelineLayout(Global::g_device, &layout_info, nullptr, &m_pipelineLayout);

    VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = get_shader_module(ROOT "shaders/gravity.comp.spv",
                                           VK_SHADER_STAGE_COMPUTE_BIT);
    shaderStage.pName = "main";

    VkComputePipelineCreateInfo computeInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    computeInfo.layout = m_pipelineLayout;
    computeInfo.stage = shaderStage;

    vkCreateComputePipelines(Global::g_device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &m_pipeline);

    //delete shader modules
}

void GravityComputePass::create_descriptor()
{
    VkDescriptorSetLayoutBinding binding1;
    binding1.binding = 0;
    binding1.descriptorCount = 1;
    binding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding binding2;
    binding2.binding = 1;
    binding2.descriptorCount = 1;
    binding2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding2.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings{binding1, binding2};

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.flags
        = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    info.bindingCount = bindings.size();
    info.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(Global::g_device, &info, nullptr, &m_descriptorSetLayout);
}

void GravityComputePass::generateData()
{
    m_bodies = {
        // glm::vec4(X, Y, Z, Mass)
        glm::vec4(0.0e0f, 0.0f, 0.0f, 1.989e30f),    // Sun
        glm::vec4(5.79e10f, 0.0f, 0.0f, 3.301e23f),  // Mercury
        glm::vec4(1.082e11f, 0.0f, 0.0f, 4.867e24f), // Venus
        glm::vec4(1.496e11f, 0.0f, 0.0f, 5.972e24f), // Earth
        glm::vec4(2.279e11f, 0.0f, 0.0f, 6.417e23f), // Mars
        glm::vec4(7.785e11f, 0.0f, 0.0f, 1.898e27f), // Jupiter
        glm::vec4(1.433e12f, 0.0f, 0.0f, 5.683e26f), // Saturn
        glm::vec4(2.877e12f, 0.0f, 0.0f, 8.681e25f), // Uranus
        glm::vec4(4.503e12f, 0.0f, 0.0f, 1.024e26f)  // Neptune
    };

    m_dataBuffer = make_buffer({m_bodies.size() * sizeof(glm::vec4),
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                    | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                    | VK_MEMORY_PROPERTY_HOST_CACHED_BIT},
                               m_bodies.data());
}