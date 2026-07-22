#ifndef PASS_H
#define PASS_H

#include "../graphics.h"
#include "global.h"

class Pass
{
public:
    explicit Pass(Graphics *graphics) : m_graphics(graphics) {}

    // If nothing is passed then default engine attachments are set
    inline void addAttachments(std::vector<Texture> *attachments = nullptr)
    {
        if (!attachments) {
            m_attachments = &Global::g_render_targets;
            m_isUsingEngineTargets = true;
        } else {
            m_attachments = attachments;
            m_isUsingEngineTargets = false;
        }
    };
    inline void addDepth(Texture *depth) { m_depth = depth; };
    inline void attachModels(std::vector<NativeModel> *models) { m_models = models; }
    inline void attachImageResources(std::vector<Texture> *textures) { m_textures = textures; }

    virtual inline void render(VkCommandBuffer *cmd, uint32_t imgIndex) = 0;
    virtual inline void read(uint32_t imgIndex) {};

    bool isUsingEngineTargets() const { return m_isUsingEngineTargets; }

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

private:
    bool m_isUsingEngineTargets = false;

    friend class Graphics;
};

#endif // PASS_H
