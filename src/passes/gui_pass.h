#ifndef GUI_PASS_H
#define GUI_PASS_H

#include "../passes/pass.h"

class GuiPass : public Pass
{
public:
    explicit GuiPass(Graphics *graphics);
    void render(VkCommandBuffer *cmd, uint32_t imgIndex) override;

    void drawDebugRect(Rect rect);
    void drawDebugLines(std::tuple<vec2, vec2> line);

private:
    void createPipeline() override;

    std::vector<Rect> m_debugRects;
    std::vector<std::tuple<vec2, vec2>> m_debugLines;
};
#endif // GUI_PASS_H
