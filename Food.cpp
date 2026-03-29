#include "Food.h"

Food Food::spawn(std::mt19937& gen) {
    std::uniform_real_distribution<float> posDist(50.f, 750.f);

    Food f;
    f.shape.setRadius(3.f);
    f.shape.setFillColor(sf::Color::Green);
    f.shape.setPosition({posDist(gen), posDist(gen)});
    return f;
}
