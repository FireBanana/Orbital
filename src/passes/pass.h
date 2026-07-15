#ifndef PASS_H
#define PASS_H

#include "../graphics.h"

class Pass
{
public:
    explicit Pass(Graphics *graphics) : m_graphics(graphics) {}

    inline void addAttachments(std::vector<Texture> *attachments) { m_attachments = attachments; };
    inline void addDepth(Texture *depth) { m_depth = depth; };
    inline void attachModels(std::vector<NativeModel> *models) { m_models = models; }
    inline void attachImageResources(std::vector<Texture> *textures) { m_textures = textures; }

    virtual inline void render(VkCommandBuffer *cmd, uint32_t imgIndex) = 0;
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
    std::vector<Texture> *m_attachments = nullptr;
    Texture *m_depth;
};

#endif // PASS_H
