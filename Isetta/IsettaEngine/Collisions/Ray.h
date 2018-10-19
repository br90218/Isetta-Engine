/*
 * Copyright (c) 2018 Isetta
 */
#pragma once
#include "Core/Math/Vector3.h"

namespace Isetta {
class Ray {
 private:
  Math::Vector3 origin, direction;

 public:
  Ray(const Math::Vector3& origin, const Math::Vector3& direction)
      : origin{origin}, direction{direction.Normalized()} {}

  inline Math::Vector3 GetOrigin() const { return origin; }
  inline Math::Vector3 GetDirection() const { return direction; }
  inline void SetOrigin(Math::Vector3 o) { origin = o; }
  inline void SetDirection(Math::Vector3 d) { direction = d.Normalized(); }

  inline Math::Vector3 GetPoint(float distance) const {
    return origin + distance * direction;
  }
};

class RaycastHit {
 private:
  class Collider* collider;
  float distance{INFINITY};
  Math::Vector3 point, normal;

  RaycastHit(class Collider* collider, float distance, Math::Vector3 point,
             Math::Vector3 normal)
      : collider{collider}, distance{distance}, point{point}, normal{normal} {}

  // friend CollisionsModule? Collider?
 public:
  inline class Collider* GetCollider() { return collider; }
  inline float GetDistance() { return distance; }
  inline Math::Vector3 GetPoint() { return point; }
  inline Math::Vector3 GetNormal() { return normal; }

  RaycastHit() = default;
  friend class Collider;
};
}  // namespace Isetta
