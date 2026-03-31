#pragma once
#include "Creature.h"
#include "Food.h"
#include <vector>
#include <random>
#include <deque>

struct SimStats {
    int   population  = 0;
    int   peakPop     = 0;
    int   foodCount   = 0;
    int   totalBirths = 0;
    float avgSize     = 0.f;
    float avgSpeed    = 0.f;
    float avgVision   = 0.f;
};

// Playback mode enum exposed so main.cpp and UI can reference it
enum class PlayMode { Playing, Paused, FastForward, Rewinding };

class Simulation {
public:
    static constexpr float WORLD_W = 860.f;
    static constexpr float WORLD_H = 640.f;

    Simulation();

    void     update();                       // normal single-step update
    void     rewindOneStep();                // restore the previous snapshot
    void     draw() const;
    SimStats getStats() const;

    int      historySize()  const { return (int)m_history.size(); }

private:
    std::mt19937          m_gen;
    std::vector<Creature> m_population;
    std::vector<Food>     m_food;
    int                   m_totalBirths = 0;
    int                   m_peakPop     = 0;

    // ── Rewind state-history system ────────────────────────────
    struct Snapshot {
        std::vector<Creature> population;
        std::vector<Food>     food;
        int                   totalBirths;
        int                   peakPop;
    };

    static constexpr int MAX_HISTORY = 600;   // ~10 s at 60 fps
    std::deque<Snapshot>  m_history;

    void saveSnapshot();
};
