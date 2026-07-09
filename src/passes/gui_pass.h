#ifndef GUI_PASS_H
#define GUI_PASS_H

#include "../passes/pass.h"

class GuiPass : public Pass
{
public:
    explicit GuiPass(Graphics *graphics);

    void render(VkCommandBuffer *cmd, uint32_t imgIndex) override;

private:
    void createPipeline() override;
};
#endif // GUI_PASS_H
