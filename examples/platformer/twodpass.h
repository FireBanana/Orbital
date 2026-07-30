#ifndef TWODPASS_H
#define TWODPASS_H

#include "gtypes.h"
#include "passes/pass.h"

class TwoDPass : public Pass
{
public:
    TwoDPass(Graphics *g);
    void render(VkCommandBuffer *cmd, uint32_t imgIndex);
    void addSprite(Sprite *sprite) { m_Sprites.push_back(sprite); }
    void addSprite(Sprite *sprite, int size)
    {
        for (auto i = 0; i < size; ++i)
            m_Sprites.push_back(&sprite[i]);
    }

protected:
    void createPipeline();
    void createDescriptor();
    void createSampler();

private:
    std::vector<Sprite *> m_Sprites;
};

#endif // TWODPASS_H
