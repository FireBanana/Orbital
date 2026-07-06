#ifndef N_BODY_COMPUTE_H
#define N_BODY_COMPUTE_H

#include "../passes/pass.h"

class GravityComputePass : public Pass
{
public:
    GravityComputePass();

    void render(VkCommandBuffer *cmd) override;

private:
    void createPipeline() override;
    void createDescriptor() override;

    void generateData();

    std::vector<glm::vec4> m_bodies;
    Buffer m_dataBuffer;
    Buffer m_readBuffer;
};

#endif // N_BODY_COMPUTE_H
