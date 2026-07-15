#include "n_body_compute.h"
#include "global.h"
#include <iostream>

GravityComputePass::GravityComputePass(Graphics *graphics)
    : Pass(graphics)
{
    createDescriptor();
    createPipeline();
    generateData();
}

void GravityComputePass::render(VkCommandBuffer *cmd, uint32_t imgIndex)
{
    vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = m_textures->at(0).view;
    imageInfo.sampler = VK_NULL_HANDLE; // Sampler is ummutable

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_dataBuffer[imgIndex].buffer;
    bufferInfo.range = m_bodies.size() * sizeof(glm::vec4);

    VkWriteDescriptorSet imageWriteSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    imageWriteSet.dstBinding = 0;
    imageWriteSet.descriptorCount = 1;
    imageWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWriteSet.pImageInfo = &imageInfo;

    VkWriteDescriptorSet bufferWriteSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    bufferWriteSet.dstBinding = 1;
    bufferWriteSet.descriptorCount = 1;
    bufferWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bufferWriteSet.pBufferInfo = &bufferInfo;

    std::array<VkWriteDescriptorSet, 2> writeSets{imageWriteSet, bufferWriteSet};

    vkCmdPushDescriptorSet(*cmd,
                           VK_PIPELINE_BIND_POINT_COMPUTE,
                           m_pipelineLayout,
                           0,
                           2,
                           writeSets.data());

    vkCmdDispatch(*cmd, m_bodies.size(), m_bodies.size(), 1);

    m_graphics->transitionBuffer(*cmd,
                     m_dataBuffer[imgIndex].buffer,
                     VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                     VK_ACCESS_2_TRANSFER_READ_BIT,
                     VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                     VK_PIPELINE_STAGE_2_COPY_BIT);

    VkBufferCopy copyRegion{};
    copyRegion.size = m_bodies.size() * sizeof(glm::vec4);

    vkCmdCopyBuffer(*cmd,
                    m_dataBuffer[imgIndex].buffer,
                    m_readBuffer[imgIndex].buffer,
                    1,
                    &copyRegion);
}

void GravityComputePass::read(uint32_t imgIndex)
{
    auto data = (glm::vec4 *) m_readBuffer[imgIndex].mappedData;
    //std::cout << data->x << std::endl;
}

void GravityComputePass::createPipeline()
{
    // VkPushConstantRange range{};
    // range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    // range.offset = 0;
    // range.size = sizeof(UniformConstants);

    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;

    // layoutInfo.pushConstantRangeCount = 1;
    // layoutInfo.pPushConstantRanges = &range;

    vkCreatePipelineLayout(Global::g_device, &layoutInfo, nullptr, &m_pipelineLayout);

    VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = m_graphics->getShaderModule(ROOT "shaders/gravity.comp.spv",
                                                     VK_SHADER_STAGE_COMPUTE_BIT);
    shaderStage.pName = "main";

    VkComputePipelineCreateInfo computeInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    computeInfo.layout = m_pipelineLayout;
    computeInfo.stage = shaderStage;

    vkCreateComputePipelines(Global::g_device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &m_pipeline);

    //delete shader modules
}

void GravityComputePass::createDescriptor()
{
    VkDescriptorSetLayoutBinding binding1{};
    binding1.binding = 0;
    binding1.descriptorCount = 1;
    binding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding binding2{};
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

    for (auto i = 0; i < Global::SWAPCHAIN_SIZE; ++i) {
        m_dataBuffer[i] = m_graphics->makeBuffer({m_bodies.size() * sizeof(glm::vec4),
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                          | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                          | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
                                     m_bodies.data());

        m_readBuffer[i] = m_graphics->makeBuffer(
            {m_bodies.size() * sizeof(glm::vec4),
             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT});
    }
}