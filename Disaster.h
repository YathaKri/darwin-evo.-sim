#pragma once
#include "raylib.h"
#include <vector>
#include <random>
#include <memory>

class Creature;

// ── Disaster types ──────────────────────────────────────────────
enum class DisasterType { Meteor, Volcano, Nuke };
enum class DisasterState { Pending, Active, Done };

// ── Sub-entities ────────────────────────────────────────────────

struct LavaPuddle {
    Vector2 position  = {0, 0};
    float   radius    = 14.f;
    int     lifetime  = 300;   // 5 seconds at 60fps
};

struct MeteorImpact {
    Vector2 position     = {0, 0};
    float   damageRadius = 100.f;  // 2 grid boxes (50px each)
    int     animTimer    = 30;     // 0.5s falling animation
    int     flashTimer   = 60;     // 1s impact flash
    bool    damageApplied = false;
};

// ── Disaster class ──────────────────────────────────────────────
class Disaster {
public:
    DisasterType  type;
    DisasterState state = DisasterState::Pending;
    Vector2       position = {0, 0};
    int           timer = 0;
    int           age   = 0;

    // ── Meteor ──
    std::vector<MeteorImpact> impacts;

    // ── Volcano ──
    std::vector<LavaPuddle> puddles;
    float volcanoRadius     = 20.f;
    int   eruptionDuration  = 600;   // 10 seconds of eruption
    std::vector<int> damagedByVolcano; // creature IDs hit by body

    // ── Nuke ──
    float innerRadius    = 200.f;  // 4 grid boxes — instant kill
    float outerRadius    = 250.f;  // 5 grid boxes — 80% HP damage
    bool  nukeDetonated  = false;
    int   explosionFlash = 0;

    // ── Factory methods ─────────────────────────────────────────
    static Disaster createMeteor(const std::vector<Vector2>& targets);
    static Disaster createVolcano(std::mt19937& gen, float worldW, float worldH);
    static Disaster createNuke(Vector2 target);

    // ── Core methods ────────────────────────────────────────────
    void update(std::mt19937& gen, float worldW, float worldH);
    void applyDamage(std::vector<std::unique_ptr<Creature>>& population);
    void draw() const;
    bool isExpired() const;

    // ── Cooldown constant ───────────────────────────────────────
    static constexpr int COOLDOWN_FRAMES = 600; // 10 seconds at 60fps
};
