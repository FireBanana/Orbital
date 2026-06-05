#ifndef FORWARD_PIPELINE_H
#define FORWARD_PIPELINE_H

#include "../global.h"
#include "../graphics.h"
namespace forward_pipeline {

inline void create_sampler()
{
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    vkCreateSampler(Global::g_device, &info, nullptr, &Global::g_sampler);
}

inline void create_descriptor()
{
    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = &Global::g_sampler;

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.flags
        = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    info.bindingCount = 1;
    info.pBindings = &binding;

    vkCreateDescriptorSetLayout(Global::g_device, &info, nullptr, &Global::g_descriptor_layout);
}

inline void create_pipeline()
{
    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(UniformConstants);

    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &Global::g_descriptor_layout;

    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &range;

    vkCreatePipelineLayout(Global::g_device, &layout_info, nullptr, &Global::g_pipeline_layout);

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
    rendering_info.pColorAttachmentFormats = &Global::FORMAT;
    rendering_info.depthAttachmentFormat = Global::DEPTH_FORMAT;

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
    info.layout = Global::g_pipeline_layout;
    info.renderPass = VK_NULL_HANDLE;
    info.subpass = 0;

    vkCreateGraphicsPipelines(Global::g_device,
                              VK_NULL_HANDLE,
                              1,
                              &info,
                              nullptr,
                              &Global::g_pipeline);

    //delete shader modules
}

} // namespace forward_pipeline
#endif // FORWARD_PIPELINE_H
