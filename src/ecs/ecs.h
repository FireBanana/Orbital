#ifndef ECS_H
#define ECS_H

#include "types.h"
#include <cstdint>

using Entity = uint32_t;

struct Transform
{
    vec3 position;
    vec3 rotation; // pitch, yaw, roll
};

class World
{
public:
    Entity createEntity()
    {
        m_entities.push_back(m_entities.size());
        return m_entities.back();
    }

private:
    std::vector<Entity> m_entities;
};

#endif