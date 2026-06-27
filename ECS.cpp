#include <iostream>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <concepts>

// Just the id that represents a game object, no data, no logic
using Entity = unsigned long;
enum : Entity { INVALID_ENTITY = 0 };

// ---------------------------------------------------------------------
// COMPONENTS (Pure Data)
// ---------------------------------------------------------------------
struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};
//----------------------------------------------------------------------
template<typename T> concept HasEntityToIdx = requires(T a) {//compile time assert helper
    a.entityToIdx;
};


class Registry { //manages EC
    
public:
    inline Entity mkEntity() { return nextEntity++; }

    template<typename T> void addComponent(Entity e, T component) {
        auto pool = getPool<T>();
        pool->insert(e, component);
    }

    template<typename T> T& getComponent(Entity e) {
        auto pool = getPool<T>();
        return pool->instances[pool->entityToIdx[e]];
    }

    template<typename T> bool hasComponent(Entity e) {
        const auto& entityToIdx = getPool<T>()->entityToIdx;
        return entityToIdx.find(e) != entityToIdx.cend();
    }

    // A simple view helper to get all entities with a specific component type
    template<typename T> std::vector<Entity> view() {
        auto pool = getPool<T>();
        static_assert(HasEntityToIdx<ComponentArray<T>>,
            "ComponentArray is missing the 'entityToIdx' map!");
        std::vector<Entity> entities;
        for (auto const& [entity, idx] : pool->entityToIdx) {
            entities.push_back(entity);
        }
        return entities;
    }

private:
    struct IComponentArray {// Abstract base class for component arrays so we can store them generically
        virtual ~IComponentArray() = default;
        virtual void remove(Entity entity) = 0;
    };
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentPools;
    Entity nextEntity = 1;

    template<typename T>  struct ComponentArray : public IComponentArray {
        std::unordered_map<Entity, size_t> entityToIdx;
        std::unordered_map<size_t, Entity> idxToEntity;
        std::vector<T> instances;

        void insert(Entity entity, T component) {
            size_t newIdx = instances.size();
            entityToIdx[entity] = newIdx;
            idxToEntity[newIdx] = entity;
            instances.push_back(component);
        }

        void remove(Entity entity) {
            if (entityToIdx.find(entity) == entityToIdx.cend()) {
                return;
            }
            size_t idxOfRemoved = entityToIdx[entity], idxOfLast = instances.size() - 1;
            instances[idxOfRemoved] = instances[idxOfLast];
            // update maps
            Entity entityOfLast = idxToEntity[idxOfLast];
            entityToIdx[entityOfLast] = idxOfRemoved;
            idxToEntity[entityOfLast] = entityOfLast;
            // erase
            instances.pop_back();
            entityToIdx.erase(entity);
            idxToEntity.erase(idxOfLast);
        }
    };

    template<typename T> std::shared_ptr<ComponentArray<T>> getPool() {
        auto typeIdx = std::type_index(typeid(T));
        if (!componentPools[typeIdx]) {
            componentPools[typeIdx] = std::make_shared<ComponentArray<T>>();
        }
        return std::static_pointer_cast<ComponentArray<T>>(componentPools[typeIdx]);
    }
};

void MovementSystem(Registry& registry, float dt) {
    // System enforces the requirement: must have BOTH Position and Velocity
    for (Entity entity : registry.view<Position>()) {
        if (!registry.hasComponent<Velocity>(entity)) {
            continue;
        }
        auto& pos = registry.getComponent<Position>(entity);
        auto const& vel = registry.getComponent<Velocity>(entity);
        pos.x += vel.dx * dt;
        pos.y += vel.dy * dt;

        std::cout << "Entity " << entity << " moved to (" << pos.x << ", " << pos.y << ")\n";
    }
}

int main() {
    Registry registry;

    Entity player = registry.mkEntity();
    registry.addComponent(player, Position(.0f, .0f));
    registry.addComponent(player, Velocity(10.0f, 10.0f));

    Entity tree = registry.mkEntity();
    registry.addComponent(tree, Position{ 10.f, 10.f });

    while (true) { // game loop
        MovementSystem(registry, 1.f);
        break; // comment this for a real loop
    }
    return 0;
}
