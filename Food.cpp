#include "Food.h"

Food Food::spawn(std::mt19937& gen, float worldW, float worldH) {
    std::uniform_real_distribution<float> posX(20.f, worldW - 20.f);
    std::uniform_real_distribution<float> posY(20.f, worldH - 20.f);

    Food f;
    f.position = {posX(gen), posY(gen)};
    f.radius   = 3.f;
    return f;
}

void Food::draw() const {
    // Soft glow around food
    DrawCircleV(position, radius * 3.f, {80, 255, 120, 18});
    DrawCircleV(position, radius * 2.f, {80, 255, 120, 35});
    // Core
    DrawCircleV(position, radius, {100, 255, 130, 255});
}
