#include "Food.h"

Food Food::spawn(std::mt19937& gen, float worldW, float worldH) {
    std::uniform_real_distribution<float> posX(20.f, worldW - 20.f);
    std::uniform_real_distribution<float> posY(20.f, worldH - 20.f);
    std::uniform_int_distribution<int> bigChance(1, 100);

    Food f;
    f.position = {posX(gen), posY(gen)};
    if (bigChance(gen) <= 10) {
        f.isBig = true;
        f.radius = 5.f;
    } else {
        f.isBig = false;
        f.radius = 3.f;
    }
    return f;
}

void Food::draw() const {
    if (isBig) {
        DrawCircleV(position, radius * 3.f, {255, 200, 80, 40});
        DrawCircleV(position, radius * 1.5f, {255, 220, 100, 100});
        // Draw a star shape or just a glowing core for big food
        DrawCircleV(position, radius, {255, 255, 120, 255});
        DrawCircleLines((int)position.x, (int)position.y, radius + 2.f, {255, 255, 200, 255});
    } else {
        // Soft glow around normal food
        DrawCircleV(position, radius * 3.f, {80, 255, 120, 18});
        DrawCircleV(position, radius * 2.f, {80, 255, 120, 35});
        // Core
        DrawCircleV(position, radius, {100, 255, 130, 255});
    }
}
