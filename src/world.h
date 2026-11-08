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
    void destroyEntity(Entity e) { m_registry.destroy(e); }
    bool isValid(Entity e) { return m_registry.valid(e); }
    uint32_t getTotalEntitySize() const { return m_registry.storage<Entity>()->free_list(); }

    template<typename T>
    T &addComponentToEntity(Entity e)
    {
        return m_registry.emplace<T>(e);
    }

    template<typename T>
    T *getComponent(Entity e)
    {
        return m_registry.try_get<T>(e);
    }

    template<typename T>
    void removeComponent(Entity e)
    {
        m_registry.erase<T>(e);
    }

    template<typename T>
    std::vector<T *> getAllComponents()
    {
        std::vector<T *> values;
        auto view = m_registry.view<T>();
        view.each([&values](T &comp) { values.push_back(&comp); });
        return values;
    }

    template<typename... T>
    std::vector<std::tuple<Entity, T *...>> getAllEntitiesWith()
    {
        std::vector<std::tuple<Entity, T *...>> result;
        auto view = m_registry.view<T...>();

        view.each([&result](Entity e, T &...comps) { result.push_back({e, &comps...}); });

        return result;
    }

    template<typename... T, typename Fn>
    void executeOnEntitiesWith(Fn &&fn)
    {
        auto view = m_registry.view<T...>();

        view.each([&fn](Entity e, T &...comps) { fn(&comps...); });
    }

private:
    entt::basic_registry<Entity> m_registry;
};

#endif
