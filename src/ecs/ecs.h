#ifndef ECS_H
#define ECS_H

#include "types.h"
#include <algorithm>
#include <cstdint>
#include <string>

using Entity = uint32_t;

// Components
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
        assert(ent != m_entities.end());
        m_entities.erase(ent);
    }

    size_t getEntityCount() const { return m_entities.size(); }

    const std::vector<Entity> &getEntities() const { return m_entities; }

    template<typename T>
    T &addComponent(Entity e, T value);

    template<typename T>
    T *getComponent(Entity e);

    template<typename T>
    bool hasComponent(Entity e) const;

    template<typename T>
    void removeComponent(Entity e);

private:
    std::vector<Entity> m_entities;

    // Components
    std::vector<Transform> m_transforms;
    std::vector<std::string> m_names;
};

template<typename T>
T &World::addComponent(Entity e, T value)
{
    if constexpr (std::is_same_v<T, Transform>) {
    } else if constexpr (std::is_same_v<T, std::string>) {
    }
}

template<typename T>
T *World::getComponent(Entity e)
{
    if constexpr (std::is_same_v<T, Transform>) {
    } else if constexpr (std::is_same_v<T, std::string>) {
    }
}

template<typename T>
bool World::hasComponent(Entity e) const
{
    if constexpr (std::is_same_v<T, Transform>) {
    } else if constexpr (std::is_same_v<T, std::string>) {
    }
}

template<typename T>
void World::removeComponent(Entity e)
{
    if constexpr (std::is_same_v<T, Transform>) {
    } else if constexpr (std::is_same_v<T, std::string>) {
    }
}

#endif
