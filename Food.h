#pragma once
#include <SFML/Graphics.hpp>
#include <random>

struct Food {
    sf::CircleShape shape;

    static Food spawn(std::mt19937& gen);
};
