#ifndef SHADOW_PASS_H
#define SHADOW_PASS_H

#include "../passes/pass.h"

struct LightConstants
{
    glm::mat4 lightVP;
    glm::mat4 model;
};

class ShadowPass : public Pass
{
public:
    ShadowPass(Graphics *g);

    void render(VkCommandBuffer *cmd, uint32_t imageIndex) override;

    void setLightDirection(glm::vec3 lightPos);

    const Texture *getShadowMap() const;
    glm::mat4 getLightVPMatrix() const;

private:
    void createPipeline() override;
    //void createSampler() override;
    //void createDescriptor() override;

    Texture *m_depthTex;
    Texture m_shadowMap;

    glm::vec3 m_lightPosition;
    glm::mat4 m_lightVP;
};

#endif // SHADOW_PASS_H
