#include "twodpass.h"
#include <iostream>

TwoDPass::TwoDPass(Graphics *g)
    : Pass(g)
{
    m_mainCharIdle = AssetLoader::loadImage(ROOT "examples/platformer/assets/walk.png");
    m_mainCharIdleTex = g->makeImage({m_mainCharIdle.width,
                                      m_mainCharIdle.height,
                                      VK_FORMAT_R8G8B8A8_SRGB,
                                      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                      VK_IMAGE_ASPECT_COLOR_BIT},
                                     &m_mainCharIdle);
    //makeimage

    addAttachments();

    createSampler();
    createDescriptor();
    createPipeline();
}

void TwoDPass::render(VkCommandBuffer *cmd, uint32_t imgIndex)
{
    VkClearValue clearColorValue{}, depthClearValue{};
    clearColorValue.color = {{0, 0, 0}};
    depthClearValue.depthStencil = {1, 0};

    VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAttachment.imageView = m_attachments->at(imgIndex).view;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColorValue;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent.width = m_graphics->getSwapchainSize().width;
    renderingInfo.renderArea.extent.height = m_graphics->getSwapchainSize().height;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport vp{};
    vp.x = 0;
    vp.y = static_cast<float>(m_graphics->getSwapchainSize().height);
    vp.width = static_cast<float>(m_graphics->getSwapchainSize().width);
    vp.height = -static_cast<float>(m_graphics->getSwapchainSize().height); // Flip viewport for Y up
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    vkCmdSetViewport(*cmd, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.extent.width = m_graphics->getSwapchainSize().width;
    scissor.extent.height = m_graphics->getSwapchainSize().height;

    vkCmdSetScissor(*cmd, 0, 1, &scissor);
    vkCmdSetCullMode(*cmd, VK_CULL_MODE_BACK_BIT);

    vkCmdBeginRendering(*cmd, &renderingInfo);

    vkCmdPushConstants(*cmd,
                       m_pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(Sprite),
                       m_Sprite);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_mainCharIdleTex.view;
    imageInfo.sampler = VK_NULL_HANDLE; // Sampler is ummutable

    VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeSet.dstBinding = 0;
    writeSet.descriptorCount = 1;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSet.pImageInfo = &imageInfo;

    vkCmdPushDescriptorSet(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &writeSet);

    vkCmdDraw(*cmd, 6, 1, 0, 0);
    vkCmdEndRendering(*cmd);
}

void TwoDPass::createPipeline()
{
    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(Sprite) + sizeof(VkExtent2D);

    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;

    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &range;

    vkCreatePipelineLayout(Global::g_device, &layoutInfo, nullptr, &m_pipelineLayout);

    // VkVertexInputBindingDescription bindingDesc{};
    // bindingDesc.binding = 0;
    // bindingDesc.stride = sizeof(vertex);
    // bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // std::array<VkVertexInputAttributeDescription, 3> attrDescription = {{
    //     {.location = 0,
    //      .binding = 0,
    //      .format = VK_FORMAT_R32G32B32_SFLOAT,
    //      .offset = offsetof(vertex, position)},
    //     {.location = 1,
    //      .binding = 0,
    //      .format = VK_FORMAT_R32G32B32_SFLOAT,
    //      .offset = offsetof(vertex, normal)},
    //     {.location = 2,
    //      .binding = 0,
    //      .format = VK_FORMAT_R32G32_SFLOAT,
    //      .offset = offsetof(vertex, uv)},
    // }};

    VkPipelineVertexInputStateCreateInfo vertexStateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    // vertexStateInfo.vertexBindingDescriptionCount = 0;
    // vertexStateInfo.vertexAttributeDescriptionCount = 0;
    //vertexStateInfo.pVertexBindingDescriptions = &bindingDesc;
    //vertexStateInfo.pVertexAttributeDescriptions = attrDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = false;

    VkPipelineRasterizationStateCreateInfo rasterInfo{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterInfo.depthClampEnable = false;
    rasterInfo.rasterizerDiscardEnable = false;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.depthBiasEnable = false;
    rasterInfo.lineWidth = 1.0;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR,
                                                 VK_DYNAMIC_STATE_CULL_MODE};

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                     | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendStateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blendStateInfo.attachmentCount = 1;
    blendStateInfo.pAttachments = &blendAttachment;

    VkPipelineViewportStateCreateInfo viewportState{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleState{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
        {{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = m_graphics->getShaderModule(ROOT "examples/platformer/shaders/sprite.vert.spv",
                                                VK_SHADER_STAGE_VERTEX_BIT),
          .pName = "main"},
         {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = m_graphics->getShaderModule(ROOT "examples/platformer/shaders/sprite.frag.spv",
                                                VK_SHADER_STAGE_FRAGMENT_BIT),
          .pName = "main"}}};

    VkPipelineRenderingCreateInfo renderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &Global::RENDER_TARGET_FORMAT;
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
}

void TwoDPass::createDescriptor()
{
    VkDescriptorSetLayoutBinding binding{};
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

void TwoDPass::createSampler()
{
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    vkCreateSampler(Global::g_device, &info, nullptr, &m_sampler);
}
