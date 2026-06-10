#ifndef FORWARD_PASS_H
#define FORWARD_PASS_H

#include "../passes/pass.h"

class ForwardPass : public Pass
{
public:
    ForwardPass();

    void render(VkCommandBuffer *cmd) override;

private:
    void create_pipeline() override;
    void create_sampler() override;
    void create_descriptor() override;
};

#endif // FORWARD_PASS_H
