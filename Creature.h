#pragma once
#include "raylib.h"
#include "Food.h"
#include <random>
#include <vector>

struct Creature {
    Vector2  position   = {0, 0};
    Vector2  velocity   = {0, 0};
    float    radius     = 6.f;
    Color    color      = WHITE;
    float    energy     = 100.f;
    float    visionRange = 100.f;   // how far the creature can "see" food

    // Trail: stores last N positions
    std::vector<Vector2> trail;
    static constexpr int TRAIL_LEN = 10;

    static constexpr float MAX_SPEED    = 4.f;   // cap velocity magnitude
    static constexpr float STEER_FORCE  = 0.12f;  // steering strength

    void     update(float worldW, float worldH, const std::vector<Food>& food);
    void     draw() const;
    Creature reproduce(std::mt19937& gen) const;
};
