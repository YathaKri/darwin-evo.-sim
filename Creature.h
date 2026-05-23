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

    // ── Big Food Buff ──
    int   bigFoodTimer = 0;       // frames remaining for infinite energy from big food

    // ── Reproduction Cooldown ──
    int   reproductionCooldown = 0; // frames before allowed to reproduce again

    // ── Trail: stores last N positions for motion-trail rendering ──
    std::vector<Vector2> trail;
    static constexpr int   TRAIL_LEN   = 10;

    static constexpr float MAX_SPEED   = 4.8f;    // velocity magnitude cap
    static constexpr float STEER_FORCE = 0.12f;  // steering strength

    // ── Mutation system ──
    struct MutationStack {
        // (a) Speed: additive sum of random 10-40% boosts, 100% inherit
        int   speedCount   = 0;
        float speedBonus   = 0.f;    // total % increase

        // (b) Vision: additive 5-15% per stack, 100% inherit, cap 1000%
        int   visionCount  = 0;
        float visionBonus  = 0.f;    // total % increase

        // (c) Stamina: -5% consumption, +5% gain per stack, 100% inherit
        int   staminaCount = 0;

        // (d) Size: +10% multiplicative per stack, 50% inherit
        int   sizeCount    = 0;

        // (e) Bioluminescence: 10% chance on any mutation, 70% inherit
        bool  hasBio       = false;
        Color neonColor    = {0, 255, 0, 255};
        int   bioCount     = 0;

        bool hasMutations() const {
            return speedCount > 0 || visionCount > 0 || staminaCount > 0 ||
                   sizeCount > 0 || hasBio;
        }
    };

    MutationStack mutations;

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
