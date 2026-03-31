#pragma once
#include "Creature.h"
#include "Food.h"
#include <vector>
#include <random>

struct SimStats {
    int   population  = 0;
    int   peakPop     = 0;
    int   foodCount   = 0;
    int   totalBirths = 0;
    float avgSize     = 0.f;
    float avgSpeed    = 0.f;
    float avgVision   = 0.f;
};

class Simulation {
public:
    static constexpr float WORLD_W = 860.f;
    static constexpr float WORLD_H = 640.f;

    Simulation();

    void     update();
    void     draw() const;
    SimStats getStats() const;

private:
    std::mt19937          m_gen;
    std::vector<Creature> m_population;
    std::vector<Food>     m_food;
    int                   m_totalBirths = 0;
    int                   m_peakPop     = 0;
};
