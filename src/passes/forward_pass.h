#ifndef FORWARD_PASS_H
#define FORWARD_PASS_H

#include "../passes/pass.h"

struct UniformConstants
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 camera;
    uint32_t frame;
};

class ForwardPass : public Pass
{
public:
    explicit ForwardPass(Graphics *graphics);
    void render(VkCommandBuffer *cmd, uint32_t imgIndex) override;

    void setCameraPosition(glm::vec3 position);

private:
    void createPipeline() override;
    void createSampler() override;
    void createDescriptor() override;

    Texture m_forwardDepth;

    glm::vec3 m_cameraPosition = glm::vec3(0., 0., 3.0);
    glm::mat4 m_model = glm::mat4(1.0f);
    glm::mat4 m_view = glm::lookAtRH(m_cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 m_projection = glm::perspectiveZO(glm::radians(60.0f),
                                                (float) 800 / 600,
                                                0.1f,
                                                1000.0f);

    UniformConstants m_constants = {m_model, m_view, m_projection, glm::vec4(1.)};
};

#endif // FORWARD_PASS_H
