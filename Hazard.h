#pragma once
#include "Entity.h"
#include <random>

// ── Hazard types ────────────────────────────────────────────────
enum class HazardType { Radiation, Toxic };

// ── Hazard: environmental danger zone (inherits Entity) ─────────
class Hazard : public Entity {
public:
    Hazard() = default;
    Hazard(Vector2 pos, float radius, HazardType type, int lifetime);

    void       update();                                   // decrement lifetime
    void       draw()        const override;               // pulsing glow
    Vector2    getPosition() const override { return m_position; }
    float      getRadius()   const override { return m_radius; }

    bool       isExpired()   const { return m_lifetime <= 0; }
    HazardType getType()     const { return m_type; }
    int        getLifetime() const { return m_lifetime; }

    // ── Damage model ──
    // RAD: high instant HP damage, no DOT
    // TOX: lower instant HP damage, but applies lingering DOT
    float      getInstantDamage() const { return m_instantDamage; }
    float      getDotDamage()     const { return m_dotDamage; }
    int        getDotDuration()   const { return m_dotDuration; }

    static Hazard spawn(std::mt19937& gen, float worldW, float worldH);

private:
    Vector2    m_position       = {0, 0};
    float      m_radius         = 50.f;
    HazardType m_type           = HazardType::Radiation;
    int        m_lifetime       = 400;

    // RAD: high instant, no DOT.  TOX: low instant, has DOT.
    float      m_instantDamage  = 1.5f;
    float      m_dotDamage      = 0.f;
    int        m_dotDuration    = 0;
};
