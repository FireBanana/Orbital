#include "gui_pass.h"
#include "../global.h"
#include "../graphics.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

GuiPass::GuiPass(Graphics *graphics)
    : Pass(graphics)
{
    addAttachments();
    // createPipeline();

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(Global::g_window, true);
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = Global::g_instance;
    init_info.PhysicalDevice = Global::g_physical_device;
    init_info.Device = Global::g_device;
    init_info.QueueFamily = Global::QUEUE_INDEX;
    init_info.Queue = Global::g_queue;
    init_info.DescriptorPoolSize = 8;
    init_info.MinImageCount = 2;
    init_info.ImageCount = Global::SWAPCHAIN_SIZE;
    init_info.UseDynamicRendering = true;
    init_info.ApiVersion = VK_API_VERSION_1_4;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &Global::RENDER_TARGET_FORMAT,
    };

    ImGui_ImplVulkan_Init(&init_info);
}

void GuiPass::render(VkCommandBuffer *cmd, uint32_t imgIndex)
{
    // VkClearValue clearColorValue{};
    // clearColorValue.color = {{0, 0, 0}};

    // vkCmdBindPipeline(*cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // VkViewport vp{};
    // vp.x = 0;
    // vp.y = static_cast<float>(Global::HEIGHT);
    // vp.width = static_cast<float>(Global::WIDTH);
    // vp.height = -static_cast<float>(Global::HEIGHT); // Flip viewport for Y up
    // vp.minDepth = 0.0f;
    // vp.maxDepth = 1.0f;

    // vkCmdSetViewport(*cmd, 0, 1, &vp);

    // VkRect2D scissor{};
    // scissor.extent.width = Global::WIDTH;
    // scissor.extent.height = Global::HEIGHT;

    // vkCmdSetScissor(*cmd, 0, 1, &scissor);
    // vkCmdSetCullMode(*cmd, VK_CULL_MODE_BACK_BIT);

    // VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    // colorAttachment.imageView = m_attachments->at(imgIndex).view;
    // colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    // colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // colorAttachment.clearValue = clearColorValue;

    // VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
    // renderingInfo.renderArea.offset = {0, 0};
    // renderingInfo.renderArea.extent.width = Global::WIDTH;
    // renderingInfo.renderArea.extent.height = Global::HEIGHT;
    // renderingInfo.layerCount = 1;
    // renderingInfo.colorAttachmentCount = 1;
    // renderingInfo.pColorAttachments = &colorAttachment;

    // vkCmdBeginRendering(*cmd, &renderingInfo);
    // vkCmdDraw(*cmd, 5, 1, 0, 0);
    // vkCmdEndRendering(*cmd);

    VkClearValue clearColorValue{};
    clearColorValue.color = {{0, 0, 0}};

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

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // your UI
    //ImGui::ShowDemoWindow(); // or your own windows

    ImDrawList *drawList = ImGui::GetBackgroundDrawList();

    const float h = ImGui::GetIO().DisplaySize.y;
    for (auto r : m_debugRects) {
        // r = {x, y, width, height} in Y-up sprite space (origin bottom-left)
        const float top = h - (r.y + r.height); // sprite top edge  -> ImGui min-Y
        const float bottom = h - r.y;           // sprite bottom    -> ImGui max-Y

        drawList->AddRect({r.x, top}, {r.x + r.width, bottom}, IM_COL32(255, 255, 0, 255));
    }

    for (auto [first, second] : m_debugLines) {
        drawList->AddLine({first.u, first.v}, {second.u, second.v}, IM_COL32(255, 255, 0, 255));
    }

    ImGui::Render(); // finalizes draw data, does NOT touch Vulkan

    vkCmdBeginRendering(*cmd, &renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
    vkCmdEndRendering(*cmd);

    m_debugRects.clear();
    m_debugLines.clear();
}

void GuiPass::drawDebugRect(Rect rect)
{
    m_debugRects.push_back(rect);
}

void GuiPass::drawDebugLines(std::tuple<vec2, vec2> line)
{
    m_debugLines.push_back(line);
}

void GuiPass::createPipeline()
{
    // Imgui makes its own pipeline
}
