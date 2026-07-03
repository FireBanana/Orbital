#ifndef GUI_PASS_H
#define GUI_PASS_H

#include "../passes/pass.h"

class GuiPass : public Pass
{
public:
    GuiPass();

    void render(VkCommandBuffer *cmd) override;

private:
    void createPipeline() override;
};
#endif // GUI_PASS_H
