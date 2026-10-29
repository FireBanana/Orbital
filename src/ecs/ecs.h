#ifndef ECS_H
#define ECS_H

#include "types.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

using Entity = uint32_t;

struct ComponentPoolBase
{
    virtual ~ComponentPoolBase() = default;
    virtual void remove(Entity e) = 0;
};

template<typename T>
struct ComponentPool : ComponentPoolBase
{
    std::unordered_map<Entity, T> values;
    void remove(Entity e) override { values.erase(e); }
};

// Components
struct Transform
{
    vec3 position;
    vec3 rotation; // pitch, yaw, roll
    vec3 scale {1, 1, 1};
};

class World
{
public:
    Entity createEntity()
    {
        m_entities.push_back(m_entityIndex);
        return m_entityIndex++;
    }

    bool isEntityValid(Entity e)
    {
        return std::find(m_entities.begin(), m_entities.end(), e) != m_entities.end();
    }

    template<typename T>
    void removeComponent(Entity e)
    {
        auto &map = getComponentPool<T>();
        map.erase(e);
    }

    // Removing entity this way will cause entities to go invalid.
    // Todo: alternate way
    void destroyEntity(Entity entity)
    {
        std::cout << "warning: remove isnt supported yet" << std::endl;
        auto ent = std::find(m_entities.begin(), m_entities.end(), entity);
        assert(ent != m_entities.end());
        m_entities.erase(ent);
        purgeEntityComponents(entity);
    }

    template<typename T>
    T *getComponent(Entity e)
    {
        auto &values = getComponentPool<T>();
        if (values.contains(e))
            return &values[e];

        return nullptr;
    }

    template<typename T>
    bool hasComponent(Entity e)
    {
        return getComponentPool<T>().contains(e);
    }

    size_t getEntityCount() const { return m_entities.size(); }

    const std::vector<Entity> &getEntities() const { return m_entities; }

    template<typename T>
    T &addComponent(Entity e)
    {
        return getComponentPool<T>()[e];
    }

private:
    uint32_t m_entityIndex = 0;
    std::vector<Entity> m_entities;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentPoolBase>> m_componentPools;

    void purgeEntityComponents(Entity e)
    {
        for (auto &[type, pool] : m_componentPools) {
            pool->remove(e);
        }
    }

    template<typename T>
    std::unordered_map<Entity, T> &getComponentPool()
    {
        auto &map = m_componentPools[typeid(T)];
        if (!map)
            map = std::make_unique<ComponentPool<T>>();

        return static_cast<ComponentPool<T> &>(*map).values;
    }
};

#endif
