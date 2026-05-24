#pragma once
#include "Creature.h"
#include "Food.h"
#include "Hazard.h"
#include "Genome.h"
#include "Disaster.h"
#include <vector>
#include <random>
#include <deque>
#include <queue>
#include <memory>
#include <string>
#include <fstream>

// ── Live simulation statistics ──────────────────────────────────
struct SimStats {
    int   population  = 0;
    int   peakPop     = 0;
    int   foodCount   = 0;
    int   totalBirths = 0;
    int   totalFights = 0;
    float avgSize     = 0.f;
    float avgSpeed    = 0.f;
    float avgVision   = 0.f;
    int   generation  = 0;
    int   hazardCount = 0;
    // Generation comparison (vs previous generation)
    float prevAvgSize   = 0.f;
    float prevAvgSpeed  = 0.f;
    float prevAvgVision = 0.f;
    bool  hasPrevGen    = false;
    // Generation 1 baseline comparison
    float gen1AvgSize   = 0.f;
    float gen1AvgSpeed  = 0.f;
    float gen1AvgVision = 0.f;
    bool  hasGen1       = false;
    
    // UI Multipliers
    int   simSpeedMult  = 1;
    int   foodDropMult  = 1;
};

// Playback mode enum exposed so main.cpp and UI can reference it
enum class PlayMode { Playing, Paused, FastForward, Rewinding };

// ── Event system using std::priority_queue ──────────────────────
struct SimEvent {
    int triggerFrame;
    enum Type { SpawnHazard, GenerationTick };
    Type type;

    bool operator>(const SimEvent& other) const {
        return triggerFrame > other.triggerFrame;
    }
};

// ── Per-generation statistics (stored in history) ───────────────
struct GenStats {
    int   generation  = 0;
    int   population  = 0;
    float avgSize     = 0.f;
    float avgSpeed    = 0.f;
    float avgVision   = 0.f;
};

// ════════════════════════════════════════════════════════════════
class Simulation {
public:
    Simulation();

    void init(int popCount, float mutationRate, int foodCount);

    void     update();
    void     rewindOneStep();
    void     draw() const;
    SimStats getStats() const;

    int      historySize() const { return (int)m_history.size(); }

    // ── Dynamic world size (for fullscreen / resize) ────────────
    float getWorldW() const { return m_worldW; }
    float getWorldH() const { return m_worldH; }
    void  setWorldSize(float w, float h) { m_worldW = w; m_worldH = h; }

    // ── Public API ──────────────────────────────────────────────
    void            saveGenomes(const std::string& filename) const;
    void            loadGenomes(const std::string& filename);
    void            generateReport(const std::string& filename) const;
    const Creature* findCreature(int id) const;

    const std::vector<std::unique_ptr<Creature>>& getCreatures() const { return m_population; }
    const std::vector<GenStats>& getGenHistory()  const { return m_genHistory; }
    int  getGeneration() const { return m_generation; }

    int  getSimSpeedMult() const { return m_simSpeedMult; }
    void setSimSpeedMult(int m) { m_simSpeedMult = m; }
    int  getFoodDropMult() const { return m_foodDropMult; }
    void setFoodDropMult(int m) { m_foodDropMult = m; }

    // ── Input Disaster API ───────────────────────────────────────
    void spawnDisaster(Disaster d);
    bool canSpawnDisaster() const { return m_disasterCooldown <= 0; }
    int  getDisasterCooldown() const { return m_disasterCooldown; }
    std::mt19937& getRng() { return m_gen; }

private:
    std::mt19937 m_gen;
    float        m_mutationRate    = 0.8f;
    int          m_currentFrame    = 0;
    int          m_nextCreatureId  = 1;
    int          m_generation      = 0;
    int          m_simSpeedMult    = 1;
    int          m_foodDropMult    = 1;

    // ── Dynamic world dimensions ────────────────────────────────
    float m_worldW = 860.f;
    float m_worldH = 640.f;

    // ── Population managed with std::unique_ptr ─────────────────
    std::vector<std::unique_ptr<Creature>> m_population;
    std::vector<Food>                      m_food;
    std::vector<std::unique_ptr<Hazard>>   m_hazards;

    // ── Input disasters (user-triggered) ─────────────────────────
    std::vector<Disaster> m_disasters;
    int m_disasterCooldown = 0;

    int m_totalBirths = 0;
    int m_totalFights = 0;
    int m_peakPop     = 0;

    // ── Collision cooldown: prevent same pair from interacting every frame
    // Key: min(id1,id2)*100000 + max(id1,id2), Value: frame when cooldown expires
    std::vector<std::pair<long long, int>> m_collisionCooldowns;
    bool isOnCooldown(int idA, int idB) const;
    void setCooldown(int idA, int idB, int frames);

    // ── Event queue ─────────────────────────────────────────────
    std::priority_queue<SimEvent,
                        std::vector<SimEvent>,
                        std::greater<SimEvent>> m_events;
    void processEvent(const SimEvent& ev);

    // ── Generation tracking ─────────────────────────────────────
    std::vector<GenStats> m_genHistory;
    void recordGeneration();

    GenStats m_gen1Stats;
    bool     m_hasGen1Stats = false;

    // ── Rewind state-history system ─────────────────────────────
    struct Snapshot {
        std::vector<Creature> population;
        std::vector<Food>     food;
        std::vector<Hazard>   hazards;
        int                   totalBirths;
        int                   totalFights;
        int                   peakPop;
        int                   currentFrame;
        int                   nextCreatureId;
        int                   generation;
        // Additional state for complete rewind
        std::vector<Disaster> disasters;
        std::vector<std::pair<long long, int>> collisionCooldowns;
    };

    static constexpr int MAX_HISTORY = 600;
    std::deque<Snapshot>  m_history;

    void saveSnapshot();
};
