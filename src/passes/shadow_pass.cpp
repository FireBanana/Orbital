#include "shadow_pass.h"

ShadowPass::ShadowPass(Graphics *g)
    : Pass{g}
{
    m_shadowMap = g->makeImage(
        {1024 * 3,
         1024 * 3,
         Global::DEPTH_FORMAT,
         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
         VK_IMAGE_ASPECT_DEPTH_BIT});

    addDepth(&m_shadowMap);

    createPipeline();
}

void ShadowPass::render(VkCommandBuffer *cmd, uint32_t imageIndex)
{
    VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    label.pLabelName = "Shadow Pass";

    vkCmdBeginDebugUtilsLabelEXT(*cmd, &label);

    VkClearValue depthClear{};
    depthClear.depthStencil = {1, 0};

    VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAttachment.imageView = m_depth->view;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent.width = 1024 * 3;
    renderingInfo.renderArea.extent.height = 1024 * 3;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachment;

    m_graphics->transitionImageLayout(*cmd,
                                      m_depth->image,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                      VK_IMAGE_ASPECT_DEPTH_BIT,
                                      VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, // srcAccessMask
                                      VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, // srcStageMask
                                      VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT);

    vkCmdBeginRendering(*cmd, &renderingInfo);
    vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport vp{};
    vp.x = 0;
    vp.y = 1024 * 3;
    vp.width = 1024 * 3;
    vp.height = -1024 * 3; // Flip viewport for Y up
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    vkCmdSetViewport(*cmd, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.extent.width = 1024 * 3;
    scissor.extent.height = 1024 * 3;

    vkCmdSetScissor(*cmd, 0, 1, &scissor);
    vkCmdSetCullMode(*cmd, VK_CULL_MODE_BACK_BIT);

    if (m_models != nullptr) {
        for (auto &model : *m_models) {
            VkDeviceSize offset{0};
            vkCmdBindVertexBuffers(*cmd, 0, 1, &model.vertex.buffer, &offset);
            vkCmdBindIndexBuffer(*cmd, model.index.buffer, offset, VK_INDEX_TYPE_UINT32);

            LightConstants c;
            c.lightVP = m_lightVP;
            c.model = model.worldTransform;

            vkCmdPushConstants(*cmd,
                               m_pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0,
                               sizeof(LightConstants),
                               &c);

            vkCmdDrawIndexed(*cmd, model.indexCount, 1, 0, 0, 0);
        }
    }

    vkCmdEndRendering(*cmd);

    m_graphics
        ->transitionImageLayout(*cmd,
                                m_depth->image,
                                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
                                VK_IMAGE_ASPECT_DEPTH_BIT,
                                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // srcAccessMask
                                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
                                    | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, // srcStageMask
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

    vkCmdEndDebugUtilsLabelEXT(*cmd);
}

void ShadowPass::setLightDirection(glm::vec3 lightPos)
{
    m_lightPosition = lightPos;

    glm::mat4 p = glm::orthoZO(-20.0, 20.0, -20.0, 20.0, 1.0, 7.5);
    glm::mat4 v = glm::lookAt(glm::normalize(m_lightPosition) * 2.0f + 1.0f,
                              glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));

    m_lightVP = p * v;
}

const Texture *ShadowPass::getShadowMap() const
{
    return &m_shadowMap;
}

glm::mat4 ShadowPass::getLightVPMatrix() const
{
    return m_lightVP;
}

void ShadowPass::createPipeline()
{
    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    range.offset = 0;
    range.size = sizeof(LightConstants);

    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    //layoutInfo.setLayoutCount = 1;
    //layoutInfo.pSetLayouts = &m_descriptorSetLayout;

    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &range;

    vkCreatePipelineLayout(Global::g_device, &layoutInfo, nullptr, &m_pipelineLayout);

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 1> attrDescription = {{
        {.location = 0,
         .binding = 0,
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = offsetof(vertex, position)},
        // {.location = 1,
        //  .binding = 0,
        //  .format = VK_FORMAT_R32G32B32_SFLOAT,
        //  .offset = offsetof(vertex, normal)},
        // {.location = 2,
        //  .binding = 0,
        //  .format = VK_FORMAT_R32G32_SFLOAT,
        //  .offset = offsetof(vertex, uv)},
    }};

    VkPipelineVertexInputStateCreateInfo vertexStateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexStateInfo.vertexBindingDescriptionCount = 1;
    vertexStateInfo.vertexAttributeDescriptionCount = (uint32_t) attrDescription.size();
    vertexStateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexStateInfo.pVertexAttributeDescriptions = attrDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = false;

    VkPipelineRasterizationStateCreateInfo rasterInfo{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterInfo.depthClampEnable = false;
    rasterInfo.rasterizerDiscardEnable = false;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.depthBiasEnable = VK_TRUE;
    rasterInfo.depthBiasConstantFactor = 1.25f;
    rasterInfo.depthBiasSlopeFactor = 1.75f;
    rasterInfo.lineWidth = 1.0;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR,
                                                 VK_DYNAMIC_STATE_CULL_MODE};

    // VkPipelineColorBlendAttachmentState blendAttachment{};
    // blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
    //                                  | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendStateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blendStateInfo.attachmentCount = 0;
    //blendStateInfo.pAttachments = &blendAttachment;

    VkPipelineViewportStateCreateInfo viewportState{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;

    VkPipelineMultisampleStateCreateInfo multisampleState{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    std::array<VkPipelineShaderStageCreateInfo, 1> shaderStages = {
        {{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = m_graphics->getShaderModule(ROOT "shaders/shadow.vert.spv",
                                                VK_SHADER_STAGE_VERTEX_BIT),
          .pName = "main"}}};

    VkPipelineRenderingCreateInfo renderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.depthAttachmentFormat = Global::DEPTH_FORMAT;

    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.pNext = &renderingInfo;
    info.stageCount = static_cast<uint32_t>(shaderStages.size());
    info.pStages = shaderStages.data();
    info.pVertexInputState = &vertexStateInfo;
    info.pInputAssemblyState = &inputAssembly;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &rasterInfo;
    info.pMultisampleState = &multisampleState;
    info.pDepthStencilState = &depthStencilState;
    info.pColorBlendState = &blendStateInfo;
    info.pDynamicState = &dynamicStateInfo;
    info.layout = m_pipelineLayout;
    info.renderPass = VK_NULL_HANDLE;
    info.subpass = 0;

    vkCreateGraphicsPipelines(Global::g_device, VK_NULL_HANDLE, 1, &info, nullptr, &m_pipeline);

    //delete shader modules
}
