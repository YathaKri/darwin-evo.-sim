#pragma once
#include "raylib.h"

// ── Abstract base class for all simulation entities ──────────────
// Demonstrates OOP Inheritance & Polymorphism:
//   Entity (abstract) → Creature (derived)
//   Entity (abstract) → Hazard   (derived)
class Entity {
public:
    virtual ~Entity() = default;

    virtual void    draw()        const = 0;   // polymorphic rendering
    virtual Vector2 getPosition() const = 0;
    virtual float   getRadius()   const = 0;
};
