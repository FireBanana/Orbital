#ifndef PASS_H
#define PASS_H

#include "../graphics.h"

class Pass
{
public:
    virtual inline void render(VkCommandBuffer *cmd) = 0;
    virtual inline void attachModels(std::vector<NativeModel> *models) { m_models = models; }
    virtual inline void attachImageResources(std::vector<Texture> *textures)
    {
        m_textures = textures;
    }

protected:
    virtual inline void createPipeline() = 0;
    virtual inline void createDescriptor() {}
    virtual inline void createSampler() {}

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkSampler m_sampler;
    std::vector<NativeModel> *m_models = nullptr;
    std::vector<Texture> *m_textures = nullptr;
};

#endif // PASS_H
