#include "Simulation.h"
#include <cmath>
#include <algorithm>

Simulation::Simulation() : m_gen(std::random_device{}()) {
    std::uniform_real_distribution<float> posDist(50.f, 750.f);
    std::uniform_real_distribution<float> velDist(-3.f, 3.f);
    std::uniform_int_distribution<int>    colorDist(50, 255);
    std::uniform_real_distribution<float> sizeDist(4.f, 10.f);

    // Spawn initial population
    for (int i = 0; i < 30; i++) {
        Creature c;
        c.shape.setRadius(sizeDist(m_gen));
        c.shape.setFillColor(sf::Color(colorDist(m_gen), colorDist(m_gen), colorDist(m_gen)));
        c.shape.setPosition({posDist(m_gen), posDist(m_gen)});
        c.velocity = {velDist(m_gen), velDist(m_gen)};
        m_population.push_back(c);
    }

    // Spawn initial food
    for (int i = 0; i < 150; i++)
        m_food.push_back(Food::spawn(m_gen));
}

void Simulation::update() {
    // Food regrowth (10% chance per frame, cap at 200)
    std::uniform_int_distribution<int> chance(1, 100);
    if (chance(m_gen) <= 10 && m_food.size() < 200)
        m_food.push_back(Food::spawn(m_gen));

    std::vector<Creature> newBabies;

    for (auto& c : m_population) {
        c.update(WORLD_W, WORLD_H);

        // Eat food
        for (auto it = m_food.begin(); it != m_food.end(); ) {
            float dx = c.shape.getPosition().x - it->shape.getPosition().x;
            float dy = c.shape.getPosition().y - it->shape.getPosition().y;
            if (std::hypot(dx, dy) < c.shape.getRadius() + it->shape.getRadius()) {
                c.energy += 40.f;
                it = m_food.erase(it);
            } else {
                ++it;
            }
        }

        // Reproduce
        if (c.energy >= 200.f) {
            newBabies.push_back(c.reproduce(m_gen));
            c.energy = 100.f;
            ++m_totalBirths;
        }
    }

    m_population.insert(m_population.end(), newBabies.begin(), newBabies.end());

    // Death
    for (auto it = m_population.begin(); it != m_population.end(); ) {
        if (it->energy <= 0) it = m_population.erase(it);
        else ++it;
    }

    if ((int)m_population.size() > m_peakPop)
        m_peakPop = (int)m_population.size();
}

void Simulation::draw(sf::RenderWindow& window) const {
    for (const auto& f : m_food)       window.draw(f.shape);
    for (const auto& c : m_population) window.draw(c.shape);
}

SimStats Simulation::getStats() const {
    SimStats s;
    s.population  = (int)m_population.size();
    s.peakPop     = m_peakPop;
    s.foodCount   = (int)m_food.size();
    s.totalBirths = m_totalBirths;

    if (!m_population.empty()) {
        for (const auto& c : m_population) {
            s.avgSize  += c.shape.getRadius();
            s.avgSpeed += std::hypot(c.velocity.x, c.velocity.y);
        }
        s.avgSize  /= m_population.size();
        s.avgSpeed /= m_population.size();
    }

    return s;
}
