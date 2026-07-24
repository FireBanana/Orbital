#ifndef TWODPASS_H
#define TWODPASS_H

#include "gtypes.h"
#include "passes/pass.h"

class TwoDPass : public Pass
{
public:
    TwoDPass(Graphics *g);
    void render(VkCommandBuffer *cmd, uint32_t imgIndex);
    void setSprite(Sprite *sprite) { m_Sprite = sprite; }

protected:
    void createPipeline();
    void createDescriptor();
    void createSampler();

private:
    Image m_mainCharIdle;
    Texture m_mainCharIdleTex;
    Sprite *m_Sprite;
};

#endif // TWODPASS_H
