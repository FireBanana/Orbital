#ifndef N_BODY_COMPUTE_H
#define N_BODY_COMPUTE_H

#include "../passes/pass.h"

class GravityComputePass : public Pass
{
public:
    GravityComputePass();

    void render(VkCommandBuffer *cmd) override;

private:
    void create_pipeline() override;
    void create_descriptor() override;

    void generateData();

    std::vector<glm::vec4> m_bodies;
    Buffer m_dataBuffer;
};

#endif // N_BODY_COMPUTE_H
