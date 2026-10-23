#ifndef ECS_H
#define ECS_H

#include "types.h"
#include <cstdint>

using Entity = uint32_t;

struct Transform
{
    vec3 position;
    vec3 rotation; // pitch, yaw, roll
    vec3 scale;
};

class World
{
public:
    Entity createEntity()
    {
        m_entities.push_back(m_entities.size());
        return m_entities.back();
    }

    void destroyEntity(Entity entity)
    {
        auto ent = std::find(m_entities.begin(), m_entities.end(), entity);
        assert(ent !=  m_entities.end());
        m_entities.erase(ent);
    }

    size_t getEntityCount() const { return m_entities.size(); }

private:
    std::vector<Entity> m_entities;

    // Components
    std::vector<Transform> m_transforms;
};

#endif
