#ifndef FORWARD_PASS_H
#define FORWARD_PASS_H

#include "../passes/pass.h"

class ForwardPass : public Pass
{
public:
    explicit ForwardPass(Graphics *graphics);
    void render(VkCommandBuffer *cmd, uint32_t imgIndex) override;

private:
    void createPipeline() override;
    void createSampler() override;
    void createDescriptor() override;

    Texture m_forwardDepth;
};

#endif // FORWARD_PASS_H
