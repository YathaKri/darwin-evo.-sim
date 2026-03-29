#include "Creature.h"
#include <cmath>
#include <algorithm>

void Creature::update(float worldW, float worldH) {
    sf::Vector2f pos = shape.getPosition();
    pos.x += velocity.x;
    pos.y += velocity.y;

    float r = shape.getRadius();
    if (pos.x <= 0 || pos.x >= worldW - (r * 2)) velocity.x *= -1;
    if (pos.y <= 0 || pos.y >= worldH - (r * 2)) velocity.y *= -1;

    shape.setPosition(pos);

    // Burn energy: bigger + faster = more costly
    float speed = std::hypot(velocity.x, velocity.y);
    energy -= (r * 0.015f) + (speed * 0.02f);
}

Creature Creature::reproduce(std::mt19937& gen) const {
    std::uniform_real_distribution<float> mutDist(-1.f, 1.f);
    std::uniform_int_distribution<int>    colMut(-20, 20);

    Creature child = *this;

    float newRadius = std::max(2.f, shape.getRadius() + mutDist(gen));
    child.shape.setRadius(newRadius);

    child.velocity.x += mutDist(gen);
    child.velocity.y += mutDist(gen);

    sf::Color c = shape.getFillColor();
    int r = std::clamp((int)c.r + colMut(gen), 0, 255);
    int g = std::clamp((int)c.g + colMut(gen), 0, 255);
    int b = std::clamp((int)c.b + colMut(gen), 0, 255);
    child.shape.setFillColor(sf::Color(r, g, b));

    child.shape.move({newRadius * 2, newRadius * 2});
    child.energy = 100.f;

    return child;
}
