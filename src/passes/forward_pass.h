#ifndef FORWARD_PASS_H
#define FORWARD_PASS_H

#include "../passes/pass.h"

class ForwardPass : public Pass
{
public:
    ForwardPass();

    void render(VkCommandBuffer *cmd) override;

private:
    void createPipeline() override;
    void createSampler() override;
    void createDescriptor() override;
};

#endif // FORWARD_PASS_H
