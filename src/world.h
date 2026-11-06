#ifndef WORLD_H
#define WORLD_H

#include "types.h"
#include <entt/entt.hpp>
#include <iostream>

using Entity = uint32_t;

class World
{
public:
    World() = default;

    Entity createEntity() { return m_registry.create(); }
    uint32_t getTotalEntitySize() const { return m_registry.storage<Entity>()->size(); }

    template<typename T>
    T &addComponentToEntity(Entity e)
    {
        return m_registry.emplace<T>(e);
    }

    template<typename T>
    std::vector<T *> getAllComponentsOfType()
    {
        std::vector<T *> values;
        auto view = m_registry.view<T>();
        view.each([&values](T &comp) { values.push_back(&comp); });
        return values;
    }

    template<typename... T>
    std::vector<std::tuple<Entity, T*...>> getAllEntitiesWith()
    {
        std::vector<std::tuple<Entity, T*...>> result;
        auto view = m_registry.view<T...>();

        view.each([&result](Entity e, T &...comps) { result.push_back({e, &comps...}); });

        return result;
    }

private:
    entt::basic_registry<Entity> m_registry;
};

#endif
