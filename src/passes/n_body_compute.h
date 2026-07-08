#ifndef N_BODY_COMPUTE_H
#define N_BODY_COMPUTE_H

#include "../global.h"
#include "../passes/pass.h"

class GravityComputePass : public Pass
{
public:
    GravityComputePass();

    void render(VkCommandBuffer *cmd, uint32_t imgIndex) override;
    void read(uint32_t imgIndex) override;

private:
    void createPipeline() override;
    void createDescriptor() override;

    void generateData();

    std::vector<glm::vec4> m_bodies;
    std::array<Buffer, Global::SWAPCHAIN_SIZE> m_dataBuffer;
    std::array<Buffer, Global::SWAPCHAIN_SIZE> m_readBuffer;
};

#endif // N_BODY_COMPUTE_H
