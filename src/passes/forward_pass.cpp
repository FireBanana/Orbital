#include "../global.h"
#include "../graphics.h"
#include "forward_pass.h"
#include <iostream>

ForwardPass::ForwardPass(Graphics *graphics)
    : Pass(graphics)
{
    m_forwardDepth = graphics->makeImage(
        {m_graphics->getSwapchainSize().width,
         m_graphics->getSwapchainSize().height,
         Global::DEPTH_FORMAT,
         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
         VK_IMAGE_ASPECT_DEPTH_BIT});

    addAttachments();
    addDepth(&m_forwardDepth);

    // Create 1x1 texture placeholder
    Image image{};
    std::vector<unsigned char> data{255, 255, 255, 255};
    image.width = 1;
    image.height = 1;
    image.channels = 4;
    image.data = data.data();
    m_placeholderTexture = graphics->makeImage(
        {1,
         1,
         VK_FORMAT_R8G8B8A8_SRGB,
         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
         VK_IMAGE_ASPECT_COLOR_BIT},
        &image);

    // uniform buffer
    m_uniformConstantBuffer = graphics->makeBuffer({
        .bufferSize = sizeof(UniformConstants),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    });

    //createSampler();
    createDescriptor();
    initialize();
    createPipeline();
}

void ForwardPass::render(VkCommandBuffer *cmd, uint32_t imgIndex)
{
    VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    label.pLabelName = "Forward Pass";

    vkCmdBeginDebugUtilsLabelEXT(*cmd, &label);

    //------------------------------

    VkClearValue clearColorValue{}, depthClearValue{};
    clearColorValue.color = {{0, 0, 0}};
    depthClearValue.depthStencil = {1, 0};

    VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAttachment.imageView = m_attachments->at(imgIndex).view;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColorValue;

    VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAttachment.imageView = m_depth->view;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue = depthClearValue;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent.width = m_graphics->getSwapchainSize().width;
    renderingInfo.renderArea.extent.height = m_graphics->getSwapchainSize().height;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    m_graphics->transitionImageLayout(
        *cmd,
        m_depth->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // srcAccessMask
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
            | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, // srcStageMask
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);

    vkCmdBeginRendering(*cmd, &renderingInfo);

    //------------------------------
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

    auto shadowBinding = m_descriptorImageLayoutbindings[static_cast<int>(TextureType::Texture0)];
    auto uniformBinding = m_descriptorBufferLayoutBindings[0]; // binding index 4

    // Update uniform buffer
    {
        m_projection = glm::perspectiveZO(glm::radians(60.0f),
                                          static_cast<float>(m_graphics->getSwapchainSize().width)
                                              / static_cast<float>(
                                                  m_graphics->getSwapchainSize().height),
                                          0.1f,
                                          1000.0f);

        m_view = glm::lookAt(m_cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        m_constants = {m_view,
                       m_projection,
                       m_lightVP,
                       glm::vec4(m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 0),
                       glm::vec4(m_lightPosition, 0.),
                       0};

        memcpy(m_uniformConstantBuffer.mappedData, &m_constants, sizeof(UniformConstants));
    }

    //forward pass
    if (m_models != nullptr) {
        int modelIndex = 0;

        for (auto &model : *m_models) {
            VkDeviceSize offset{0};
            vkCmdBindVertexBuffers(*cmd, 0, 1, &model.vertex.buffer, &offset);
            vkCmdBindIndexBuffer(*cmd, model.index.buffer, offset, VK_INDEX_TYPE_UINT32);

            std::vector<VkDescriptorImageInfo> imageInfos{};
            imageInfos.reserve(m_descriptorImageLayoutbindings.size());
            std::vector<VkDescriptorBufferInfo> bufferInfos{};
            bufferInfos.reserve(m_descriptorBufferLayoutBindings.size());
            std::vector<VkWriteDescriptorSet> writeSets{};
            writeSets.reserve(m_descriptorImageLayoutbindings.size()
                              + m_descriptorBufferLayoutBindings.size());

            // Add shadow map
            {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
                imageInfo.imageView = m_shadowMap->view;
                imageInfo.sampler = VK_NULL_HANDLE; // Sampler is ummutable
                imageInfos.push_back(imageInfo);

                VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writeSet.dstBinding = shadowBinding.layoutBinding.binding;
                writeSet.descriptorCount = 1;
                writeSet.descriptorType = shadowBinding.layoutBinding.descriptorType;
                writeSet.pImageInfo = &imageInfos.back();
                writeSets.push_back(writeSet);
            }

            // Attach uniform buffer
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = m_uniformConstantBuffer.buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(UniformConstants);
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writeSet.dstBinding = uniformBinding.binding;
                writeSet.descriptorCount = 1;
                writeSet.descriptorType = uniformBinding.descriptorType;
                writeSet.pBufferInfo = &bufferInfos.back();
                writeSets.push_back(writeSet);
            }

            // Images
            for (auto &layoutBinding : m_descriptorImageLayoutbindings) {
                if (layoutBinding.textureType >= TextureType::Texture0)
                    continue;

                Texture tex = model.textures[layoutBinding.textureType];
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = model.textures[layoutBinding.textureType].view
                                              == VK_NULL_HANDLE
                                          ? m_placeholderTexture.view
                                          : model.textures[layoutBinding.textureType].view;
                imageInfo.sampler = VK_NULL_HANDLE; // Sampler is ummutable
                imageInfos.push_back(imageInfo);

                VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writeSet.dstBinding = layoutBinding.layoutBinding.binding;
                writeSet.descriptorCount = 1;
                writeSet.descriptorType = layoutBinding.layoutBinding.descriptorType;
                writeSet.pImageInfo = &imageInfos.back();
                writeSets.push_back(writeSet);
            }

            //Buffers
            for (auto &layoutBinding : m_descriptorBufferLayoutBindings) {
                if (layoutBinding.binding
                    == 4) //Skipping the uniform buffer, have type like texture?
                    continue;

                VkDescriptorBufferInfo bufferInfo{};
                //bufferInfo.buffer = m_uniformConstantBuffer.buffer;
                //bufferInfo.offset = {0};
                //bufferInfo.range = sizeof(UniformConstants);
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writeSet.dstBinding = layoutBinding.binding;
                writeSet.descriptorCount = 1;
                writeSet.descriptorType = layoutBinding.descriptorType;
                writeSet.pBufferInfo = &bufferInfos.back();
                writeSets.push_back(writeSet);
            }

            ModelConstants mc{glm::translate(model.worldTransform,
                                             glm::vec3(model.position.x,
                                                       model.position.y,
                                                       model.position.z)),
                              glm::vec4{model.roughness, 0., 0., 0.}};

            vkCmdPushConstants(*cmd,
                               m_pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(ModelConstants),
                               &mc);

            vkCmdPushDescriptorSet(*cmd,
                                   VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   m_pipelineLayout,
                                   0,
                                   writeSets.size(),
                                   writeSets.data());

            vkCmdDrawIndexed(*cmd, model.indexCount, 1, 0, 0, 0);

            modelIndex++;
        }
    }

    vkCmdEndRendering(*cmd);

    vkCmdEndDebugUtilsLabelEXT(*cmd);
}

void ForwardPass::setShadowMap(const Texture *tex)
{
    m_shadowMap = tex;
}

void ForwardPass::setLightVPMatrix(glm::mat4 lightVP)
{
    m_lightVP = lightVP;
}

void ForwardPass::setCameraPosition(glm::vec3 position)
{
    m_cameraPosition = position;
}

void ForwardPass::setLightDirection(glm::vec3 lightPos)
{
    m_lightPosition = lightPos;
}

void ForwardPass::createPipeline()
{
    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(ModelConstants);

    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;

    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &range;

    vkCreatePipelineLayout(Global::g_device, &layoutInfo, nullptr, &m_pipelineLayout);

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attrDescription = {{
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
    rasterInfo.depthBiasEnable = false;
    rasterInfo.lineWidth = 1.0;

    std::vector<VkDynamicState> dynamicStates
        = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_CULL_MODE};

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
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;

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
          .module
          = m_graphics->getShaderModule(ROOT "shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
          .pName = "main"},
         {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = m_graphics->getShaderModule(
              ROOT "shaders/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
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

    //delete shader modules
};

void ForwardPass::createDescriptor()
{
    addImageDescriptor(0, SamplerType::Color, VK_SHADER_STAGE_FRAGMENT_BIT, TextureType::Diffuse);
    addImageDescriptor(1, SamplerType::Color, VK_SHADER_STAGE_FRAGMENT_BIT, TextureType::Normal);

    addImageDescriptor(2,
                       SamplerType::Color,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       TextureType::MetallicRoughness);

    // Depth texture
    addImageDescriptor(3, SamplerType::Depth, VK_SHADER_STAGE_FRAGMENT_BIT, TextureType::Texture0);

    addBufferDescriptor(4,
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
}
