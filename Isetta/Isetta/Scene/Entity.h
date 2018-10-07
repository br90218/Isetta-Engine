/*
 * Copyright (c) 2018 Isetta
 */
#pragma once
#include <bitset>
#include <typeindex>
#include <typeinfo>
#include <vector>
#include "Scene/Transform.h"
#include "Util.h"
#include "Core/Memory/MemoryManager.h"

namespace Isetta {
class Entity {
  enum class EntityAttributes { IS_ACTIVE, NEED_DESTROY, IS_TRANSFORM_DIRTY };

  friend class RenderModule;
  friend class Level;

  std::vector<std::type_index> componentTypes;
  std::vector<class Component*> components;
  Transform transform;
  std::vector<Entity*> children;

  void PropagateTransform();

  void OnEnable();
  void CheckStart();
  void GuiUpdate();
  void Update();
  void FixedUpdate();
  void LastUpdate();
  void CheckDestroy();
  void OnDisable();

  std::bitset<3> attributes;

  StringId entityID;

  void SetAttribute(EntityAttributes attr, bool value);
  bool GetAttribute(EntityAttributes attr) const;

 public:
  Entity(std::string name);
  ~Entity();

  static void Destroy(Entity* entity);

  void SetActive(bool inActive);
  bool GetActive() const;

  template <typename T, typename... Args>
  T* AddComponent(bool isActive, Args&&... args);
  template <typename T>
  T* GetComponent();
  template <typename T>
  std::vector<T*> GetComponents();

  void SetTransform(const Math::Vector3& inPosition,
                    const Math::Vector3& inRotation,
                    const Math::Vector3& inScale);
  const Transform& GetTransform() const;
};

template <typename T, typename... Args>
T* Entity::AddComponent(bool isActive, Args&&... args) {
  // if (!std::is_base_of<class Component, T>::value) {
  //   throw std::logic_error(Util::StrFormat("%s is not a derived class from
  //   Component class",
  //                                          typeid(T).name));
  // }
  // static_assert(std::is_base_of<class Component, T>::value, "T must be
  // derived from Component class.");
  if constexpr (!std::is_base_of<class Component, T>::value) {
    throw std::logic_error(Util::StrFormat(
        "%s is not a derived class from Component class", typeid(T).name));
  } else {
    T* component = MemoryManager::NewOnFreeList<T>(std::forward<Args>(args)...);
    component->SetActive(isActive);
    component->owner = this;
    if (isActive) {
      component->OnEnable();
    }
    componentTypes.emplace_back(std::type_index(typeid(T)));
    components.emplace_back(component);
    return component;
  }
}

template <typename T>
T* Entity::GetComponent() {
  for (int i = 0; i < componentTypes.size(); i++) {
    // might not work since `==` operator has problems with shared libraries on
    // some platform if so, use SID(type_info.name)) instead
    if (std::type_index(typeid(T)) == componentTypes[i]) {
      return static_cast<T*>(components[i]);
    }
  }
  return nullptr;
}

template <typename T>
std::vector<T*> Entity::GetComponents() {
  std::vector<T*> returnValue;
  returnValue.reserve(componentTypes.size());
  for (int i = 0; i < componentTypes.size(); i++) {
    if (std::type_index(typeid(T)) == componentTypes[i]) {
      returnValue.emplace_back(static_cast<T*>(components[i]));
    }
  }
  return returnValue;
}
}  // namespace Isetta
