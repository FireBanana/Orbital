#ifndef FORWARD_PASS_H
#define FORWARD_PASS_H

#include "../passes/pass.h"

struct UniformConstants // Uniform buffer
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 lightVP;
    glm::vec4 camera;
    glm::vec4 lightPos;
    uint32_t frame;
};

struct ModelConstants // Push constant
{
    glm::mat4 model;
    glm::vec4 roughMetal;
};

class ForwardPass : public Pass
{
public:
    explicit ForwardPass(Graphics *graphics);
    void render(VkCommandBuffer *cmd, uint32_t imgIndex) override;

    void setShadowMap(const Texture *tex);
    void setLightVPMatrix(glm::mat4 lightVP);
    void setCameraPosition(glm::vec3 position);
    void setLightPosition(glm::vec3 lightPos);

private:
    void createPipeline() override;
    void createDescriptor() override;

    Texture m_forwardDepth;
    Texture m_placeholderTexture;
    const Texture *m_shadowMap = nullptr;

    glm::vec3 m_cameraPosition = glm::vec3(0., 0., 3.0);
    glm::mat4 m_view = glm::lookAtRH(m_cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 m_projection = glm::perspectiveZO(glm::radians(60.0f),
                                                (float) 800 / 600,
                                                0.1f,
                                                1000.0f);
    glm::mat4 m_lightVP;
    glm::vec3 m_lightPosition;

    UniformConstants m_constants = {m_view, m_projection, m_lightVP, glm::vec4(1.)};
    Buffer m_uniformConstantBuffer;
};

#endif // FORWARD_PASS_H
