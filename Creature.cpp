#include "Creature.h"
#include <cmath>
#include <algorithm>
#include <limits>

void Creature::update(float worldW, float worldH, const std::vector<Food>& food) {
    // Save position to trail before moving
    trail.push_back(position);
    if ((int)trail.size() > TRAIL_LEN)
        trail.erase(trail.begin());

    // ── Vision & Steering ──────────────────────────────────────
    // Find closest food within visionRange
    float closestDist = std::numeric_limits<float>::max();
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

    // Steer toward closest visible food
    if (closestFood) {
        float dx = closestFood->position.x - position.x;
        float dy = closestFood->position.y - position.y;
        float d  = std::sqrt(dx * dx + dy * dy);
        if (d > 0.001f) {
            // Desired direction (unit vector) scaled by STEER_FORCE
            float steerX = (dx / d) * STEER_FORCE;
            float steerY = (dy / d) * STEER_FORCE;
            velocity.x += steerX;
            velocity.y += steerY;
        }
    }

    // Clamp to MAX_SPEED
    float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (speed > MAX_SPEED) {
        velocity.x = (velocity.x / speed) * MAX_SPEED;
        velocity.y = (velocity.y / speed) * MAX_SPEED;
        speed = MAX_SPEED;
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
    // Bigger + faster = more costly, and better eyes cost extra
    float visionCost = visionRange * 0.0003f;   // ~0.03 per frame at range 100
    energy -= (radius * 0.015f) + (speed * 0.02f) + visionCost;
}

void Creature::draw() const {
    // --- Vision ring (very faint debugging arc) ---
    DrawCircleLines((int)position.x, (int)position.y, visionRange, {color.r, color.g, color.b, 16});

    // --- Trail ---
    for (int i = 0; i < (int)trail.size(); i++) {
        float t       = (float)i / TRAIL_LEN;          // 0 = oldest, 1 = newest
        unsigned char a = (unsigned char)(t * 80);      // fade out
        float r       = radius * t * 0.6f;
        DrawCircleV(trail[i], r, {color.r, color.g, color.b, a});
    }

    // --- Glow layers (additive-style via alpha stacking) ---
    float energyRatio = std::clamp(energy / 200.f, 0.f, 1.f);

    // Outer soft glow
    DrawCircleV(position, radius * 2.8f, {color.r, color.g, color.b, 18});
    DrawCircleV(position, radius * 2.0f, {color.r, color.g, color.b, 28});
    DrawCircleV(position, radius * 1.5f, {color.r, color.g, color.b, 45});

    // --- Body ---
    DrawCircleV(position, radius, color);

    // --- Energy ring ---
    // Green (high) -> Yellow (mid) -> Red (low)
    Color ringColor;
    if (energyRatio > 0.5f) {
        // Green to Yellow
        float t    = (energyRatio - 0.5f) * 2.f;
        ringColor  = {
            (unsigned char)(255 * (1.f - t)),
            255,
            0,
            200
        };
    } else {
        // Yellow to Red
        float t    = energyRatio * 2.f;
        ringColor  = {
            255,
            (unsigned char)(255 * t),
            0,
            200
        };
    }

    // Draw ring as a thick circle outline
    float ringR = radius + 3.f;
    DrawCircleLines((int)position.x, (int)position.y, ringR, ringColor);
    DrawCircleLines((int)position.x, (int)position.y, ringR + 1, {ringColor.r, ringColor.g, ringColor.b, 100});
}

Creature Creature::reproduce(std::mt19937& gen) const {
    std::uniform_real_distribution<float> mutDist(-1.f, 1.f);
    std::uniform_int_distribution<int>    colMut(-20, 20);
    std::uniform_real_distribution<float> visionMut(-10.f, 10.f);  // vision mutation

    Creature child   = *this;
    child.trail.clear();

    // Mutate size
    child.radius = std::max(2.f, radius + mutDist(gen));

    // Mutate velocity
    child.velocity.x += mutDist(gen);
    child.velocity.y += mutDist(gen);

    // Mutate color
    int r = std::clamp((int)color.r + colMut(gen), 30, 255);
    int g = std::clamp((int)color.g + colMut(gen), 30, 255);
    int b = std::clamp((int)color.b + colMut(gen), 30, 255);
    child.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};

    // Mutate visionRange (clamped so it doesn't go negative or absurdly large)
    child.visionRange = std::clamp(visionRange + visionMut(gen), 20.f, 300.f);

    // Offset so they don't spawn on top of each other
    child.position.x += child.radius * 2;
    child.position.y += child.radius * 2;
    child.energy = 100.f;

    return child;
}
