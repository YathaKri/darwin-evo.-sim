#include "Creature.h"
#include <cmath>
#include <algorithm>
#include <limits>

// ── Apply genome traits to expressed fields ─────────────────────
void Creature::applyGenome() {
    radius      = genome.size;
    color       = genome.getColor();
    visionRange = genome.visionRange;
    maxHp       = 80.f + genome.size * 4.f;   // bigger = more HP
    hp          = maxHp;
}

// ── Per-frame update ────────────────────────────────────────────
void Creature::update(float worldW, float worldH, const std::vector<Food>& food,
                      const std::vector<std::unique_ptr<Creature>>& population,
                      std::mt19937& gen) {
    ++age;

    // ── DOT tick (lingering Toxic damage) ───────────────────────
    if (dotTimer > 0) {
        hp -= dotDamage;
        --dotTimer;
        if (dotTimer <= 0) { dotDamage = 0.f; }
    }

    // ── Flash timer countdown ───────────────────────────────────
    if (flashTimer > 0) --flashTimer;

    // Save position to trail before moving
    trail.push_back(position);
    if ((int)trail.size() > TRAIL_LEN)
        trail.erase(trail.begin());

    // ── MATE PATHFINDING ────────────────────────────────────────
    const Creature* targetMate = nullptr;
    if (mateTargetId != -1) {
        for (const auto& other : population) {
            if (other->id == mateTargetId) {
                targetMate = other.get();
                break;
            }
        }
    }

    if (targetMate) {
        // Move straight towards mate
        float dx = targetMate->position.x - position.x;
        float dy = targetMate->position.y - position.y;
        float d  = std::sqrt(dx * dx + dy * dy);
        if (d > 0.001f) {
            float moveSpeed = std::min(genome.speed, MAX_SPEED);
            velocity.x = (dx / d) * moveSpeed;
            velocity.y = (dy / d) * moveSpeed;
        }
    } else {
        // ── FOOD PATHFINDING: find closest food within visionRange ──
        float closestDist       = std::numeric_limits<float>::max();
        const Food* closestFood = nullptr;

        for (const auto& f : food) {
            float dx = f.position.x - position.x;
            float dy = f.position.y - position.y;
            float d  = std::sqrt(dx * dx + dy * dy);
            if (d < visionRange && d < closestDist) {
                closestDist = d;
                closestFood = &f;
            }
        }

        if (closestFood) {
            // ── NATURAL PATHFINDING: set velocity directly toward food ─
            float dx = closestFood->position.x - position.x;
            float dy = closestFood->position.y - position.y;
            float d  = std::sqrt(dx * dx + dy * dy);
            if (d > 0.001f) {
                float moveSpeed = std::min(genome.speed, MAX_SPEED);
                velocity.x = (dx / d) * moveSpeed;
                velocity.y = (dy / d) * moveSpeed;
            }
        } else {
            // ── WANDERING: no food visible, random walk ─────────────
            std::uniform_real_distribution<float> nudge(-0.15f, 0.15f);
            velocity.x += nudge(gen);
            velocity.y += nudge(gen);

            float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            float wanderMax = genome.speed * 0.5f;
            if (speed > wanderMax && speed > 0.001f) {
                velocity.x = (velocity.x / speed) * wanderMax;
                velocity.y = (velocity.y / speed) * wanderMax;
            }
            if (speed < 0.3f) {
                std::uniform_real_distribution<float> angleDist(0.f, 6.28318f);
                float angle = angleDist(gen);
                velocity.x = 0.5f * std::cos(angle);
                velocity.y = 0.5f * std::sin(angle);
            }
        }
    }

    // ── Movement ───────────────────────────────────────────────
    position.x += velocity.x;
    position.y += velocity.y;

    // Bounce off walls
    if (position.x - radius <= 0 || position.x + radius >= worldW) velocity.x *= -1;
    if (position.y - radius <= 0 || position.y + radius >= worldH) velocity.y *= -1;

    position.x = std::clamp(position.x, radius, worldW - radius);
    position.y = std::clamp(position.y, radius, worldH - radius);

    // ── Energy burn ────────────────────────────────────────────
    float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    float visionCost = visionRange * 0.0003f;
    energy -= (radius * 0.012f) + (speed * 0.015f) + visionCost;
}

// ── Draw helper: polygon at position ────────────────────────────
static void drawPolygon(Vector2 center, float r, int sides, Color col) {
    for (int i = 0; i < sides; i++) {
        float a1 = (2.f * 3.14159f / sides) * i - 3.14159f / 2.f;
        float a2 = (2.f * 3.14159f / sides) * (i + 1) - 3.14159f / 2.f;
        Vector2 v1 = {center.x + r * std::cos(a1), center.y + r * std::sin(a1)};
        Vector2 v2 = {center.x + r * std::cos(a2), center.y + r * std::sin(a2)};
        DrawTriangle(center, v1, v2, col);
    }
}

static void drawPolygonLines(Vector2 center, float r, int sides, Color col) {
    for (int i = 0; i < sides; i++) {
        float a1 = (2.f * 3.14159f / sides) * i - 3.14159f / 2.f;
        float a2 = (2.f * 3.14159f / sides) * (i + 1) - 3.14159f / 2.f;
        Vector2 v1 = {center.x + r * std::cos(a1), center.y + r * std::sin(a1)};
        Vector2 v2 = {center.x + r * std::cos(a2), center.y + r * std::sin(a2)};
        DrawLineV(v1, v2, col);
    }
}

// ── Rendering ───────────────────────────────────────────────────
void Creature::draw() const {
    // --- Vision ring (very faint) ---
    DrawCircleLines((int)position.x, (int)position.y, visionRange,
                    {color.r, color.g, color.b, 12});

    // --- Trail ---
    for (int i = 0; i < (int)trail.size(); i++) {
        float t       = (float)i / TRAIL_LEN;
        unsigned char a = (unsigned char)(t * 60);
        float trailR  = radius * t * 0.5f;
        DrawCircleV(trail[i], trailR, {color.r, color.g, color.b, a});
    }

    // ── Determine render color (flash white if taking damage) ───
    Color renderColor = color;
    if (flashTimer > 0) {
        // Lerp toward white based on flash intensity
        float t = (float)flashTimer / 8.f;
        renderColor.r = (unsigned char)(color.r + (255 - color.r) * t);
        renderColor.g = (unsigned char)(color.g + (255 - color.g) * t);
        renderColor.b = (unsigned char)(color.b + (255 - color.b) * t);
    }

    // --- Glow layers ---
    DrawCircleV(position, radius * 2.5f, {renderColor.r, renderColor.g, renderColor.b, 15});
    DrawCircleV(position, radius * 1.8f, {renderColor.r, renderColor.g, renderColor.b, 25});

    // --- Mating indicator ---
    if (mateTargetId != -1) {
        DrawCircleLines((int)position.x, (int)position.y, radius + 4.f, {255, 105, 180, 200});
        DrawCircleLines((int)position.x, (int)position.y, radius + 5.f, {255, 105, 180, 100});
    }

    // --- Body shape (varies by genome) ---
    int sides = 0;
    switch (shape) {
        case ShapeType::Circle:
            DrawCircleV(position, radius, renderColor);
            break;
        case ShapeType::Triangle:
            sides = 3;
            break;
        case ShapeType::Diamond:
            sides = 4;
            break;
        case ShapeType::Pentagon:
            sides = 5;
            break;
        case ShapeType::Hexagon:
            sides = 6;
            break;
    }
    if (sides > 0) {
        drawPolygon(position, radius, sides, renderColor);
    }

    // --- HP bar (above creature) ---
    float hpRatio = std::clamp(hp / maxHp, 0.f, 1.f);
    float barW    = radius * 2.5f;
    float barH    = 2.5f;
    float barX    = position.x - barW / 2.f;
    float barY    = position.y - radius - 6.f;

    // Background
    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, {40, 40, 40, 180});
    // Fill
    Color hpCol;
    if (hpRatio > 0.5f)
        hpCol = {80, 255, 100, 220};
    else if (hpRatio > 0.25f)
        hpCol = {255, 200, 60, 220};
    else
        hpCol = {255, 60, 60, 220};
    DrawRectangle((int)barX, (int)barY, (int)(barW * hpRatio), (int)barH, hpCol);

    // --- Energy ring ---
    float energyRatio = std::clamp(energy / 200.f, 0.f, 1.f);
    Color ringColor;
    if (energyRatio > 0.5f) {
        float t   = (energyRatio - 0.5f) * 2.f;
        ringColor = {
            (unsigned char)(255 * (1.f - t)),
            255, 0, 160
        };
    } else {
        float t   = energyRatio * 2.f;
        ringColor = {
            255,
            (unsigned char)(255 * t),
            0, 160
        };
    }

    float ringR = radius + 2.f;
    if (sides > 0) {
        drawPolygonLines(position, ringR, sides, ringColor);
    } else {
        DrawCircleLines((int)position.x, (int)position.y, ringR, ringColor);
    }

    // --- DOT indicator (small pulsing poison icon) ---
    if (dotTimer > 0) {
        float pulse = 0.5f + 0.5f * std::sin((float)age * 0.3f);
        unsigned char pa = (unsigned char)(120 + 80 * pulse);
        DrawCircleV({position.x + radius + 3.f, position.y - radius},
                    2.5f, {160, 40, 255, pa});
    }
}

// ── Combat power: determines fight outcomes ─────────────────────
float Creature::getCombatPower() const {
    // Bigger + more HP + more energy = stronger fighter
    return radius * 2.f + hp * 0.3f + energy * 0.1f;
}

// ── Reproduction: two parents combine genes ─────────────────────
Creature Creature::reproduce(std::mt19937& gen, const Creature& other) const {
    Creature child;

    // ── Gene crossover: randomly pick each trait from parent A or B ──
    std::uniform_int_distribution<int> coin(0, 1);

    child.genome.size        = coin(gen) ? genome.size        : other.genome.size;
    child.genome.speed       = coin(gen) ? genome.speed       : other.genome.speed;
    child.genome.visionRange = coin(gen) ? genome.visionRange : other.genome.visionRange;
    child.genome.r           = coin(gen) ? genome.r           : other.genome.r;
    child.genome.g           = coin(gen) ? genome.g           : other.genome.g;
    child.genome.b           = coin(gen) ? genome.b           : other.genome.b;

    // ── Then mutate ──
    child.genome = child.genome.mutate(gen, mutationRate);
    child.applyGenome();
    child.trail.clear();

    // ── Inherit shape from one parent randomly, with small chance to change ──
    child.shape = coin(gen) ? shape : other.shape;
    std::uniform_real_distribution<float> shapeMutChance(0.f, 1.f);
    if (shapeMutChance(gen) < 0.08f) {
        std::uniform_int_distribution<int> shapeDist(0, 4);
        child.shape = (ShapeType)shapeDist(gen);
    }

    // Set child velocity from genome speed with random direction
    std::uniform_real_distribution<float> angleDist(0.f, 6.28318f);
    float angle = angleDist(gen);
    child.velocity.x = child.genome.speed * std::cos(angle);
    child.velocity.y = child.genome.speed * std::sin(angle);

    // Spawn between the two parents
    child.position.x = (position.x + other.position.x) / 2.f;
    child.position.y = (position.y + other.position.y) / 2.f;

    child.energy       = 80.f;
    child.mutationRate = mutationRate;
    child.id           = 0;   // assigned by Simulation
    child.age          = 0;

    return child;
}
