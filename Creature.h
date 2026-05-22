#pragma once
#include "Entity.h"
#include "Genome.h"
#include "Food.h"
#include <random>
#include <vector>
#include <memory>

// ── Organism shape types (inherited via Genome) ─────────────────
enum class ShapeType { Circle, Triangle, Diamond, Pentagon, Hexagon };

// ── Creature: derived from Entity (demonstrates inheritance) ────
struct Creature : public Entity {
    // ── Genetic blueprint ──
    Genome genome;

    // ── Expressed traits (synced from genome at birth via applyGenome) ──
    Vector2  position    = {0, 0};
    Vector2  velocity    = {0, 0};
    float    radius      = 6.f;
    Color    color       = WHITE;
    float    energy      = 100.f;
    float    hp          = 100.f;       // health points (separate from energy)
    float    maxHp       = 100.f;
    float    visionRange = 100.f;

    // ── Shape (inherited) ──
    ShapeType shape      = ShapeType::Circle;

    // ── Identity & lifecycle ──
    int   id           = 0;       // unique, assigned by Simulation
    int   age          = 0;       // frames alive
    float mutationRate = 0.8f;    // per-trait mutation probability
    int   mateTargetId = -1;      // ID of mate if pathfinding to reproduce

    // ── Hazard immunity (inherited from survivors) ──
    bool  toxImmune    = false;   // immune to Toxic hazards
    bool  radImmune    = false;   // immune to Radiation hazards

    // ── Flee state (escaping hazards) ──
    bool    fleeing     = false;
    Vector2 fleeTarget  = {0, 0};

    // ── Badge check ──
    bool hasBadge() const { return toxImmune || radImmune; }

    // ── Damage flash ──
    int   flashTimer   = 0;       // frames remaining for white flash

    // ── DOT (damage over time from Toxic hazards) ──
    float dotDamage    = 0.f;     // damage per frame from lingering poison
    int   dotTimer     = 0;       // frames remaining for DOT

    // ── Trail: stores last N positions for motion-trail rendering ──
    std::vector<Vector2> trail;
    static constexpr int   TRAIL_LEN   = 10;

    static constexpr float MAX_SPEED   = 4.8f;    // velocity magnitude cap
    static constexpr float STEER_FORCE = 0.12f;  // steering strength

    // ── Sync expressed traits from genome ──
    void applyGenome();

    // ── Core methods ──
    void     update(float worldW, float worldH, const std::vector<Food>& food,
                    const std::vector<std::unique_ptr<Creature>>& population,
                    std::mt19937& gen);
    Creature reproduce(std::mt19937& gen, const Creature& other) const;

    // ── Combat ──
    float    getCombatPower() const;

    // ── Entity overrides (polymorphism) ──
    void    draw()        const override;
    Vector2 getPosition() const override { return position; }
    float   getRadius()   const override { return radius; }
};
