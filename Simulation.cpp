#include "Simulation.h"
#include <cmath>
#include <algorithm>

Simulation::Simulation() : m_gen(std::random_device{}()) {
    std::uniform_real_distribution<float> posX(50.f, WORLD_W - 50.f);
    std::uniform_real_distribution<float> posY(50.f, WORLD_H - 50.f);
    std::uniform_real_distribution<float> velDist(-3.f, 3.f);
    std::uniform_int_distribution<int>    colorDist(50, 255);
    std::uniform_real_distribution<float> sizeDist(4.f, 10.f);

    for (int i = 0; i < 30; i++) {
        Creature c;
        c.position = {posX(m_gen), posY(m_gen)};
        c.velocity = {velDist(m_gen), velDist(m_gen)};
        c.radius   = sizeDist(m_gen);
        c.color    = {
            (unsigned char)colorDist(m_gen),
            (unsigned char)colorDist(m_gen),
            (unsigned char)colorDist(m_gen),
            255
        };
        m_population.push_back(c);
    }

    for (int i = 0; i < 150; i++)
        m_food.push_back(Food::spawn(m_gen, WORLD_W, WORLD_H));
}

void Simulation::update() {
    // Food regrowth
    std::uniform_int_distribution<int> chance(1, 100);
    if (chance(m_gen) <= 10 && (int)m_food.size() < 200)
        m_food.push_back(Food::spawn(m_gen, WORLD_W, WORLD_H));

    std::vector<Creature> newBabies;

    for (auto& c : m_population) {
        c.update(WORLD_W, WORLD_H, m_food);

        // Eat food
        for (auto it = m_food.begin(); it != m_food.end(); ) {
            float dx   = c.position.x - it->position.x;
            float dy   = c.position.y - it->position.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < c.radius + it->radius) {
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
    for (auto it = m_population.begin(); it != m_population.end(); )
        it = (it->energy <= 0) ? m_population.erase(it) : ++it;

    if ((int)m_population.size() > m_peakPop)
        m_peakPop = (int)m_population.size();
}

void Simulation::draw() const {
    for (const auto& f : m_food)       f.draw();
    for (const auto& c : m_population) c.draw();
}

SimStats Simulation::getStats() const {
    SimStats s;
    s.population  = (int)m_population.size();
    s.peakPop     = m_peakPop;
    s.foodCount   = (int)m_food.size();
    s.totalBirths = m_totalBirths;

    if (!m_population.empty()) {
        for (const auto& c : m_population) {
            s.avgSize   += c.radius;
            s.avgSpeed  += std::sqrt(c.velocity.x * c.velocity.x + c.velocity.y * c.velocity.y);
            s.avgVision += c.visionRange;
        }
        s.avgSize   /= m_population.size();
        s.avgSpeed  /= m_population.size();
        s.avgVision /= m_population.size();
    }

    return s;
}
