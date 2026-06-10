#ifndef PASS_H
#define PASS_H

#include "../graphics.h"

class Pass
{
public:
    virtual inline void render(VkCommandBuffer *cmd) = 0;
    virtual inline void attach_models(std::vector<NativeModel> *models) { m_models = models; }

protected:
    virtual inline void create_pipeline() = 0;
    virtual inline void create_descriptor() {}
    virtual inline void create_sampler() {}

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkSampler m_sampler;
    std::vector<NativeModel> *m_models = nullptr;
};

#endif // PASS_H
