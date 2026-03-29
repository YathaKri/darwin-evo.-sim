#pragma once
#include "raylib.h"
#include <random>
#include <vector>

struct Creature {
    Vector2  position  = {0, 0};
    Vector2  velocity  = {0, 0};
    float    radius    = 6.f;
    Color    color     = WHITE;
    float    energy    = 100.f;

    // Trail: stores last N positions
    std::vector<Vector2> trail;
    static constexpr int TRAIL_LEN = 10;

    void     update(float worldW, float worldH);
    void     draw() const;
    Creature reproduce(std::mt19937& gen) const;
};
