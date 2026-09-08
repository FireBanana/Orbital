#ifndef GUI_PASS_H
#define GUI_PASS_H

#include "../passes/pass.h"

class GuiPass : public Pass
{
public:
    explicit GuiPass(Graphics *graphics);
    void render(VkCommandBuffer *cmd, uint32_t imgIndex) override;

    void drawDebugRect(GuiRect rect);
    void drawDebugLines(std::tuple<vec2, vec2> line);

    void addSlider(std::string name, float min, float max, std::function<void(float)> valueChanged);

private:
    void createPipeline() override;

    std::vector<GuiRect> m_debugRects;
    std::vector<GuiSlider> m_sliders;
    std::vector<std::tuple<vec2, vec2>> m_debugLines;
};
#endif // GUI_PASS_H
