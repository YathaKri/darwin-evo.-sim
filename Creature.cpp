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

    // ── Flash & Cooldown timers ───────────────────────────────────
    if (flashTimer > 0) --flashTimer;
    if (reproductionCooldown > 0) --reproductionCooldown;

    // Save position to trail before moving
    trail.push_back(position);
    if ((int)trail.size() > TRAIL_LEN)
        trail.erase(trail.begin());

    // ── FLEE: highest priority — escaping a hazard ──────────────
    if (fleeing) {
        float dx = fleeTarget.x - position.x;
        float dy = fleeTarget.y - position.y;
        float d  = std::sqrt(dx * dx + dy * dy);
        if (d > 0.001f) {
            // Sprint at full MAX_SPEED to escape
            velocity.x = (dx / d) * MAX_SPEED;
            velocity.y = (dy / d) * MAX_SPEED;
        }
        // fleeing flag is cleared by Simulation once creature exits hazard
    }
    // ── MATE PATHFINDING ────────────────────────────────────────
    else if (mateTargetId != -1) {
        const Creature* targetMate = nullptr;
        for (const auto& other : population) {
            if (other->id == mateTargetId) {
                targetMate = other.get();
                break;
            }
        }
        if (targetMate) {
            float dx = targetMate->position.x - position.x;
            float dy = targetMate->position.y - position.y;
            float d  = std::sqrt(dx * dx + dy * dy);
            if (d > 0.001f) {
                float moveSpeed = std::min(genome.speed, MAX_SPEED);
                velocity.x = (dx / d) * moveSpeed;
                velocity.y = (dy / d) * moveSpeed;
            }
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

    // ── Energy burn (mutation-aware) ────────────────────────────
    float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    float visionCost = visionRange * 0.0003f;
    float baseCost = (radius * 0.012f) + (speed * 0.015f) + visionCost;

    // (c) Stamina mutation: each stack reduces consumption by 5%
    float staminaReduction = 1.f - (mutations.staminaCount * 0.05f);
    if (staminaReduction < 0.1f) staminaReduction = 0.1f;  // minimum 10% cost

    // (d) Size mutation: more size stacks = faster drain
    float sizeDrainPenalty = 1.f + (mutations.sizeCount * 0.08f);

    if (bigFoodTimer > 0) {
        bigFoodTimer--;
        energy = 200.f; // Keep max energy while buff is active
    } else {
        energy -= baseCost * staminaReduction * sizeDrainPenalty;
    }
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
    if (mutations.hasBio) {
        // Render a large neon glowing ring behind the creature
        DrawCircleV(position, radius * 3.5f, {mutations.neonColor.r, mutations.neonColor.g, mutations.neonColor.b, 20});
        DrawCircleV(position, radius * 2.5f, {mutations.neonColor.r, mutations.neonColor.g, mutations.neonColor.b, 40});
    } else {
        DrawCircleV(position, radius * 2.5f, {renderColor.r, renderColor.g, renderColor.b, 15});
        DrawCircleV(position, radius * 1.8f, {renderColor.r, renderColor.g, renderColor.b, 25});
    }

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

    // --- BADGES: immunity indicators ---
    float badgeOffsetX = 0.f;

    // TOX badge: silver diamond (4-sided polygon)
    if (toxImmune) {
        float bx = position.x - radius - 4.f - badgeOffsetX;
        float by = position.y - radius - 2.f;
        float bs = 3.f;
        // Draw a small filled diamond (silver)
        Vector2 top   = {bx,      by - bs};
        Vector2 right = {bx + bs, by};
        Vector2 bot   = {bx,      by + bs};
        Vector2 left  = {bx - bs, by};
        DrawTriangle(top, left, bot, {192, 192, 210, 230});
        DrawTriangle(top, bot, right, {192, 192, 210, 230});
        DrawLineV(top, right, {220, 220, 235, 255});
        DrawLineV(right, bot, {220, 220, 235, 255});
        DrawLineV(bot, left, {220, 220, 235, 255});
        DrawLineV(left, top, {220, 220, 235, 255});
        badgeOffsetX += 10.f;
    }

    // RAD badge: gold star (5-pointed)
    if (radImmune) {
        float bx = position.x - radius - 4.f - badgeOffsetX;
        float by = position.y - radius - 2.f;
        float bs = 3.5f;
        Color gold = {255, 215, 0, 240};
        Color goldEdge = {255, 235, 100, 255};
        // 5-pointed star
        for (int i = 0; i < 5; i++) {
            float a1 = (2.f * 3.14159f / 5.f) * i - 3.14159f / 2.f;
            float a2 = (2.f * 3.14159f / 5.f) * (i + 1) - 3.14159f / 2.f;
            float aMid = (a1 + a2) / 2.f;
            Vector2 outer1 = {bx + bs * std::cos(a1), by + bs * std::sin(a1)};
            Vector2 outer2 = {bx + bs * std::cos(a2), by + bs * std::sin(a2)};
            Vector2 inner  = {bx + bs * 0.4f * std::cos(aMid), by + bs * 0.4f * std::sin(aMid)};
            DrawTriangle({bx, by}, outer1, inner, gold);
            DrawTriangle({bx, by}, inner, outer2, gold);
            DrawLineV(outer1, inner, goldEdge);
            DrawLineV(inner, outer2, goldEdge);
        }
    }

    // --- MUTATION ICONS (right side of creature) ---
    float iconX = position.x + radius + 4.f;
    float iconY = position.y - radius - 2.f;
    float iconStep = 9.f;

    // (a) Speed: boot symbol (two small right-pointing triangles)
    for (int s = 0; s < mutations.speedCount && s < 3; s++) {
        float ix = iconX;
        float iy = iconY + s * iconStep;
        Color bootCol = {100, 200, 255, 230};
        DrawTriangle({ix, iy - 2.f}, {ix, iy + 2.f}, {ix + 4.f, iy}, bootCol);
        DrawTriangle({ix + 2.f, iy - 1.5f}, {ix + 2.f, iy + 2.5f}, {ix + 5.f, iy + 0.5f}, bootCol);
    }
    if (mutations.speedCount > 0) iconX += 8.f;

    // (b) Vision: eye symbol (ellipse + dot)
    for (int v = 0; v < mutations.visionCount && v < 3; v++) {
        float ix = iconX;
        float iy = iconY + v * iconStep;
        Color eyeCol = {255, 220, 100, 230};
        DrawEllipse((int)ix, (int)iy, 4.f, 2.5f, {eyeCol.r, eyeCol.g, eyeCol.b, 80});
        DrawEllipseLines((int)ix, (int)iy, 4.f, 2.5f, eyeCol);
        DrawCircleV({ix, iy}, 1.2f, eyeCol);
    }
    if (mutations.visionCount > 0) iconX += 10.f;

    // (c) Stamina: lightning bolt (zigzag)
    for (int st = 0; st < mutations.staminaCount && st < 3; st++) {
        float ix = iconX;
        float iy = iconY + st * iconStep;
        Color boltCol = {255, 255, 80, 240};
        DrawLineEx({ix, iy - 3.f}, {ix - 1.5f, iy}, 1.2f, boltCol);
        DrawLineEx({ix - 1.5f, iy}, {ix + 1.5f, iy}, 1.2f, boltCol);
        DrawLineEx({ix + 1.5f, iy}, {ix, iy + 3.f}, 1.2f, boltCol);
    }
    if (mutations.staminaCount > 0) iconX += 7.f;

    // (d) Size: body symbol (small filled circle)
    for (int sz = 0; sz < mutations.sizeCount && sz < 3; sz++) {
        float ix = iconX;
        float iy = iconY + sz * iconStep;
        Color sizeCol = {200, 120, 255, 220};
        DrawCircleV({ix, iy}, 2.5f, sizeCol);
        DrawCircleLines((int)ix, (int)iy, 2.5f, {230, 160, 255, 255});
    }
    if (mutations.sizeCount > 0) iconX += 7.f;

    // (e) Bioluminescence: neon green square
    if (mutations.hasBio) {
        DrawRectangle((int)(iconX - 2.5f), (int)(iconY - 2.5f), 5, 5, mutations.neonColor);
        DrawRectangleLines((int)(iconX - 2.5f), (int)(iconY - 2.5f), 5, 5, {255, 255, 255, 120});
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

    // ── Inherit shape from one parent randomly ──
    child.shape = coin(gen) ? shape : other.shape;
    std::uniform_real_distribution<float> chance(0.f, 1.f);
    if (chance(gen) < 0.08f) {
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
    child.id           = 0;
    child.age          = 0;

    // ── Immunity inheritance (70% per gene, independent) ──
    if (toxImmune || other.toxImmune)
        child.toxImmune = (chance(gen) < 0.7f);
    if (radImmune || other.radImmune)
        child.radImmune = (chance(gen) < 0.7f);

    // ════════════════════════════════════════════════════════════
    //  MUTATION INHERITANCE (pool from both parents)
    // ════════════════════════════════════════════════════════════

    // (a) Speed: 100% inheritance — take max from parents
    if (mutations.speedCount > 0 || other.mutations.speedCount > 0) {
        if (mutations.speedCount >= other.mutations.speedCount) {
            child.mutations.speedCount = mutations.speedCount;
            child.mutations.speedBonus = mutations.speedBonus;
        } else {
            child.mutations.speedCount = other.mutations.speedCount;
            child.mutations.speedBonus = other.mutations.speedBonus;
        }
    }

    // (b) Vision: 100% inheritance — take max from parents, cap 1000%
    if (mutations.visionCount > 0 || other.mutations.visionCount > 0) {
        if (mutations.visionBonus >= other.mutations.visionBonus) {
            child.mutations.visionCount = mutations.visionCount;
            child.mutations.visionBonus = mutations.visionBonus;
        } else {
            child.mutations.visionCount = other.mutations.visionCount;
            child.mutations.visionBonus = other.mutations.visionBonus;
        }
    }

    // (c) Stamina: 100% inheritance — take max from parents
    if (mutations.staminaCount > 0 || other.mutations.staminaCount > 0) {
        child.mutations.staminaCount = std::max(mutations.staminaCount, other.mutations.staminaCount);
    }

    // (d) Size: 50% inheritance — multiplicative effect
    int parentSizeCount = std::max(mutations.sizeCount, other.mutations.sizeCount);
    if (parentSizeCount > 0) {
        child.mutations.sizeCount = (chance(gen) < 0.5f) ? parentSizeCount : 0;
    }

    // (e) Bioluminescence: 70% inheritance
    if (mutations.hasBio || other.mutations.hasBio) {
        if (chance(gen) < 0.7f) {
            child.mutations.hasBio = true;
            // Inherit a parent's neon color
            child.mutations.neonColor = mutations.hasBio ? mutations.neonColor : other.mutations.neonColor;
            child.mutations.bioCount = std::max(mutations.bioCount, other.mutations.bioCount);
        }
    }

    // ════════════════════════════════════════════════════════════
    //  NEW MUTATION ROLL (5% chance per reproduction)
    // ════════════════════════════════════════════════════════════
    if (chance(gen) < 0.05f) {
        // Pick one of 4 primary categories (a-d)
        std::uniform_int_distribution<int> mutType(0, 3);
        int picked = mutType(gen);

        switch (picked) {
            case 0: { // (a) Speed: +10% to +40%
                std::uniform_real_distribution<float> speedRoll(10.f, 40.f);
                float bonus = speedRoll(gen);
                child.mutations.speedCount++;
                child.mutations.speedBonus += bonus;
                break;
            }
            case 1: { // (b) Vision: +5% to +15%, cap 1000%
                std::uniform_real_distribution<float> visRoll(5.f, 15.f);
                float bonus = visRoll(gen);
                child.mutations.visionCount++;
                child.mutations.visionBonus = std::min(child.mutations.visionBonus + bonus, 1000.f);
                break;
            }
            case 2: { // (c) Stamina: -5% consumption, +5% gain
                child.mutations.staminaCount++;
                break;
            }
            case 3: { // (d) Size: +10% multiplicative
                child.mutations.sizeCount++;
                break;
            }
        }

        // (e) Bioluminescence: 10% bonus chance on ANY mutation
        if (chance(gen) < 0.10f) {
            child.mutations.hasBio = true;
            child.mutations.bioCount++;
            // Random neon color
            std::uniform_int_distribution<int> neonRoll(0, 4);
            Color neons[] = {
                {0, 255, 100, 255},   // neon green
                {255, 0, 255, 255},   // neon magenta
                {0, 255, 255, 255},   // neon cyan
                {255, 255, 0, 255},   // neon yellow
                {255, 100, 0, 255}    // neon orange
            };
            child.mutations.neonColor = neons[neonRoll(gen)];
        }
    }

    // ── Apply mutation effects to child's stats ──
    // (a) Speed bonus
    if (child.mutations.speedCount > 0) {
        child.genome.speed *= (1.f + child.mutations.speedBonus / 100.f);
    }
    // (b) Vision bonus (additive, capped)
    if (child.mutations.visionCount > 0) {
        child.visionRange = child.genome.visionRange * (1.f + child.mutations.visionBonus / 100.f);
    }
    // (d) Size mutation: +10% multiplicative per stack
    if (child.mutations.sizeCount > 0) {
        float sizeMult = 1.f;
        for (int i = 0; i < child.mutations.sizeCount; i++)
            sizeMult *= 1.1f;
        child.radius *= sizeMult;
        child.maxHp = 80.f + child.radius * 4.f;
        child.hp = child.maxHp;
    }
    // (e) Bioluminescence body color override
    if (child.mutations.hasBio) {
        child.color = child.mutations.neonColor;
    }

    // Recompute velocity with potentially mutated speed
    child.velocity.x = child.genome.speed * std::cos(angle);
    child.velocity.y = child.genome.speed * std::sin(angle);

    return child;
}
