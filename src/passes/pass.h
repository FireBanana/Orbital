#ifndef PASS_H
#define PASS_H

#include "../graphics.h"

class Pass
{
public:
    explicit Pass(Graphics *graphics) : m_graphics(graphics) {}

    virtual inline void setup() = 0;
    virtual inline void render(VkCommandBuffer *cmd, uint32_t imgIndex) = 0;
    virtual inline void attachModels(std::vector<NativeModel> *models) { m_models = models; }
    virtual inline void attachImageResources(std::vector<Texture> *textures)
    {
        m_textures = textures;
    }
    virtual inline void read(uint32_t imgIndex) {};

protected:
    virtual inline void createPipeline() = 0;
    virtual inline void createDescriptor() {}
    virtual inline void createSampler() {}

    Graphics *m_graphics = nullptr;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkSampler m_sampler;
    std::vector<NativeModel> *m_models = nullptr;
    std::vector<Texture> *m_textures = nullptr;
};

#endif // PASS_H
