#include "forward_pass.h"
#include "../global.h"
#include "../graphics.h"

ForwardPass::ForwardPass()
{
    create_sampler();
    create_descriptor();
    create_pipeline();
}

void ForwardPass::render(VkCommandBuffer *cmd)
{
    vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport vp{};
    vp.x = 0;
    vp.y = static_cast<float>(Global::HEIGHT);
    vp.width = static_cast<float>(Global::WIDTH);
    vp.height = -static_cast<float>(Global::HEIGHT); // Flip viewport for Y up
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    vkCmdSetViewport(*cmd, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.extent.width = Global::WIDTH;
    scissor.extent.height = Global::HEIGHT;

    vkCmdSetScissor(*cmd, 0, 1, &scissor);
    vkCmdSetCullMode(*cmd, VK_CULL_MODE_BACK_BIT);

    //forward pass
    if (m_models != nullptr) {
        for (auto &model : *m_models) {
            // updates go here

            Global::g_view = glm::lookAt(Global::g_camera_position,
                                         glm::vec3(0, 0, 0),
                                         glm::vec3(0, 1, 0));
            Global::g_constants = {glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(model.position.x,
                                                            model.position.y,
                                                            model.position.z)),
                                   Global::g_view,
                                   Global::g_projection,
                                   glm::vec4(Global::g_camera_position.x,
                                             Global::g_camera_position.y,
                                             Global::g_camera_position.z,
                                             0),
                                   0};
            //

            vkCmdPushConstants(*cmd,
                               m_pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(UniformConstants),
                               &Global::g_constants);

            VkDeviceSize offset{0};
            vkCmdBindVertexBuffers(*cmd, 0, 1, &model.vertex.buffer, &offset);
            vkCmdBindIndexBuffer(*cmd, model.index.buffer, offset, VK_INDEX_TYPE_UINT16);

            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = model.texture.view;
            image_info.sampler = VK_NULL_HANDLE; // Sampler is ummutable

            VkWriteDescriptorSet write_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write_set.dstBinding = 0;
            write_set.descriptorCount = 1;
            write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_set.pImageInfo = &image_info;

            vkCmdPushDescriptorSet(*cmd,
                                   VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   m_pipelineLayout,
                                   0,
                                   1,
                                   &write_set);

            vkCmdDrawIndexed(*cmd, model.index_count, 1, 0, 0, 0);
        }
    }
}

void ForwardPass::create_pipeline()
{
    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(UniformConstants);

    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &m_descriptorSetLayout;

    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &range;

    vkCreatePipelineLayout(Global::g_device, &layout_info, nullptr, &m_pipelineLayout);

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
    rendering_info.pColorAttachmentFormats = &Global::RENDER_TARGET_FORMAT;
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
    info.layout = m_pipelineLayout;
    info.renderPass = VK_NULL_HANDLE;
    info.subpass = 0;

    vkCreateGraphicsPipelines(Global::g_device, VK_NULL_HANDLE, 1, &info, nullptr, &m_pipeline);

    //delete shader modules
};
void ForwardPass::create_sampler()
{
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    vkCreateSampler(Global::g_device, &info, nullptr, &m_sampler);
}

void ForwardPass::create_descriptor()
{
    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = &m_sampler;

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.flags
        = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    info.bindingCount = 1;
    info.pBindings = &binding;

    vkCreateDescriptorSetLayout(Global::g_device, &info, nullptr, &m_descriptorSetLayout);
}
