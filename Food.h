#pragma once
#include "raylib.h"
#include <random>

struct Food {
    Vector2 position = {0, 0};
    float   radius   = 3.f;

    void draw() const;
    static Food spawn(std::mt19937& gen, float worldW, float worldH);
};
