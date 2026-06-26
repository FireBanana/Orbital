#include "n_body_compute.h"
#include "../global.h"

GravityComputePass::GravityComputePass()
{
    create_descriptor();
    create_pipeline();
}

void GravityComputePass::render(VkCommandBuffer *cmd)
{
    vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView = m_textures->at(0).view;
    image_info.sampler = VK_NULL_HANDLE; // Sampler is ummutable

    VkWriteDescriptorSet write_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write_set.dstBinding = 0;
    write_set.descriptorCount = 1;
    write_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_set.pImageInfo = &image_info;

    vkCmdPushDescriptorSet(*cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &write_set);

    vkCmdDispatch(*cmd, 900, 800, 1);
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
    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.flags
        = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    info.bindingCount = 1;
    info.pBindings = &binding;

    vkCreateDescriptorSetLayout(Global::g_device, &info, nullptr, &m_descriptorSetLayout);
}