#pragma once
#include <SFML/Graphics.hpp>
#include <random>

struct Creature {
    sf::CircleShape shape;
    sf::Vector2f    velocity;
    float           energy = 100.f;

    void     update(float worldW, float worldH);
    Creature reproduce(std::mt19937& gen) const;
};
