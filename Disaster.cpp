#include "Disaster.h"
#include "Creature.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

// ═══════════════════════════════════════════════════════════════
//  FACTORY METHODS
// ═══════════════════════════════════════════════════════════════

Disaster Disaster::createMeteor(const std::vector<Vector2>& targets) {
    Disaster d;
    d.type  = DisasterType::Meteor;
    d.state = DisasterState::Pending;
    d.timer = 0;

    for (const auto& t : targets) {
        MeteorImpact imp;
        imp.position     = t;
        imp.damageRadius = 100.f; // 2 grid boxes
        imp.animTimer    = 30;    // 0.5s fall
        imp.flashTimer   = 90;    // 1.5s flash
        imp.damageApplied = false;
        d.impacts.push_back(imp);
    }

    if (!targets.empty()) d.position = targets[0];
    return d;
}

Disaster Disaster::createVolcano(std::mt19937& gen, float worldW, float worldH) {
    Disaster d;
    d.type  = DisasterType::Volcano;
    d.state = DisasterState::Pending;
    d.timer = 300; // 5 second countdown

    std::uniform_real_distribution<float> posX(100.f, worldW - 100.f);
    std::uniform_real_distribution<float> posY(100.f, worldH - 100.f);
    d.position = {posX(gen), posY(gen)};
    d.volcanoRadius    = 22.f;
    d.eruptionDuration = 420; // 7 seconds of eruption

    return d;
}

Disaster Disaster::createNuke(Vector2 target) {
    Disaster d;
    d.type  = DisasterType::Nuke;
    d.state = DisasterState::Pending;
    d.timer = 300; // 5 second countdown

    d.position    = target;
    d.innerRadius = 200.f; // 4 grid boxes
    d.outerRadius = 250.f; // 5 grid boxes
    d.nukeDetonated  = false;
    d.explosionFlash = 0;

    return d;
}

// ═══════════════════════════════════════════════════════════════
//  UPDATE
// ═══════════════════════════════════════════════════════════════

void Disaster::update(std::mt19937& gen, float worldW, float worldH) {
    ++age;

    switch (type) {
        case DisasterType::Meteor: {
            bool allDone = true;
            for (auto& imp : impacts) {
                if (imp.animTimer > 0) {
                    --imp.animTimer;
                    allDone = false;
                } else if (!imp.damageApplied) {
                    // Damage will be applied in applyDamage()
                    allDone = false;
                } else if (imp.flashTimer > 0) {
                    --imp.flashTimer;
                    allDone = false;
                }
            }
            if (allDone) state = DisasterState::Done;
            else         state = DisasterState::Active;
            break;
        }

        case DisasterType::Volcano: {
            if (state == DisasterState::Pending) {
                --timer;
                if (timer <= 0) {
                    state = DisasterState::Active;
                }
            } else if (state == DisasterState::Active) {
                --eruptionDuration;

                // Spawn a lava puddle every ~30 frames
                if (age % 30 == 0) {
                    LavaPuddle lp;
                    std::uniform_real_distribution<float> offsetX(-120.f, 120.f);
                    std::uniform_real_distribution<float> offsetY(-120.f, 120.f);
                    lp.position.x = std::clamp(position.x + offsetX(gen), 10.f, worldW - 10.f);
                    lp.position.y = std::clamp(position.y + offsetY(gen), 10.f, worldH - 10.f);
                    lp.radius   = 14.f;
                    lp.lifetime = 300; // 5 seconds
                    puddles.push_back(lp);
                }

                // Update puddle lifetimes
                for (auto& p : puddles) --p.lifetime;
                puddles.erase(
                    std::remove_if(puddles.begin(), puddles.end(),
                        [](const LavaPuddle& p) { return p.lifetime <= 0; }),
                    puddles.end());

                if (eruptionDuration <= 0) {
                    state = DisasterState::Done;
                }
            }
            break;
        }

        case DisasterType::Nuke: {
            if (state == DisasterState::Pending) {
                --timer;
                if (timer <= 0) {
                    state = DisasterState::Active;
                    nukeDetonated = false; // will be set true after damage
                    explosionFlash = 180;  // 3 seconds of explosion visual
                }
            } else if (state == DisasterState::Active) {
                --explosionFlash;
                if (explosionFlash <= 0) {
                    state = DisasterState::Done;
                }
            }
            break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  APPLY DAMAGE
// ═══════════════════════════════════════════════════════════════

void Disaster::applyDamage(std::vector<std::unique_ptr<Creature>>& population) {
    switch (type) {
        case DisasterType::Meteor: {
            for (auto& imp : impacts) {
                if (imp.animTimer <= 0 && !imp.damageApplied) {
                    for (auto& c : population) {
                        float dx   = c->position.x - imp.position.x;
                        float dy   = c->position.y - imp.position.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist < imp.damageRadius) {
                            c->hp -= c->maxHp * 0.7f; // 70% of max HP
                            c->flashTimer = 8;
                        }
                    }
                    imp.damageApplied = true;
                }
            }
            break;
        }

        case DisasterType::Volcano: {
            if (state != DisasterState::Active) break;

            // Volcano body: 40 damage per second (continuous)
            for (auto& c : population) {
                float dx   = c->position.x - position.x;
                float dy   = c->position.y - position.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < volcanoRadius + c->radius) {
                    c->hp -= 40.f / 60.f;
                    // Flash every few frames to show continuous burning
                    if (age % 10 == 0) c->flashTimer = 8;
                }
            }

            // Lava puddles: 5 damage per second (continuous)
            for (const auto& puddle : puddles) {
                for (auto& c : population) {
                    float dx   = c->position.x - puddle.position.x;
                    float dy   = c->position.y - puddle.position.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < puddle.radius + c->radius) {
                        c->hp -= 15.f / 60.f; // 15 damage per second
                    }
                }
            }
            break;
        }

        case DisasterType::Nuke: {
            if (state == DisasterState::Active && !nukeDetonated) {
                for (auto& c : population) {
                    float dx   = c->position.x - position.x;
                    float dy   = c->position.y - position.y;
                    float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist < innerRadius) {
                        // Inner zone: instant kill
                        c->hp     = 0.f;
                        c->energy = 0.f;
                    } else if (dist < outerRadius) {
                        // Outer ring: 80% of current HP as damage
                        c->hp -= c->hp * 0.8f;
                        c->flashTimer = 8;
                    }
                }
                nukeDetonated = true;
            }
            break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  DRAW
// ═══════════════════════════════════════════════════════════════

void Disaster::draw() const {
    switch (type) {

        // ── METEOR ──────────────────────────────────────────────
        case DisasterType::Meteor: {
            for (const auto& imp : impacts) {
                if (imp.animTimer > 0) {
                    // Falling meteor animation
                    float t = (float)imp.animTimer / 30.f;
                    float meteorY = imp.position.y - 250.f * t;
                    float meteorX = imp.position.x + 40.f * t;

                    // Meteor body
                    DrawCircleV({meteorX, meteorY}, 5.f, {255, 130, 30, 255});
                    DrawCircleV({meteorX, meteorY}, 8.f, {255, 80, 20, 80});

                    // Trail
                    for (int j = 1; j <= 5; j++) {
                        float tj = t + j * 0.06f;
                        float ty = imp.position.y - 250.f * tj;
                        float tx = imp.position.x + 40.f * tj;
                        unsigned char a = (unsigned char)(100 - j * 18);
                        DrawCircleV({tx, ty}, 4.f - j * 0.6f, {255, 100, 20, a});
                    }

                    // Target marker on ground
                    DrawCircleLines((int)imp.position.x, (int)imp.position.y,
                                    imp.damageRadius, {255, 80, 30, 40});
                    DrawCircleLines((int)imp.position.x, (int)imp.position.y,
                                    8.f, {255, 100, 30, 120});

                } else if (imp.flashTimer > 0) {
                    float flashRatio = (float)imp.flashTimer / 90.f;

                    // Damage radius zone (fading)
                    unsigned char za = (unsigned char)(35 * flashRatio);
                    DrawCircleV(imp.position, imp.damageRadius, {255, 60, 20, za});

                    // Expanding shockwave ring
                    float ringR = imp.damageRadius * (1.f + (1.f - flashRatio) * 0.2f);
                    DrawCircleLines((int)imp.position.x, (int)imp.position.y,
                                    ringR, {255, 120, 40, (unsigned char)(180 * flashRatio)});

                    // Crater
                    DrawCircleV(imp.position, 10.f, {40, 20, 10, (unsigned char)(200 * flashRatio)});
                    DrawCircleV(imp.position, 6.f,  {100, 50, 15, (unsigned char)(255 * flashRatio)});

                    // Sparks
                    if (flashRatio > 0.5f) {
                        float sparkT = (flashRatio - 0.5f) * 2.f;
                        for (int s = 0; s < 8; s++) {
                            float angle = s * 0.785f + age * 0.02f;
                            float sr = 15.f + (1.f - sparkT) * 40.f;
                            Vector2 sp = {imp.position.x + sr * std::cos(angle),
                                          imp.position.y + sr * std::sin(angle)};
                            DrawCircleV(sp, 2.f, {255, 180, 50, (unsigned char)(200 * sparkT)});
                        }
                    }
                }
            }
            break;
        }

        // ── VOLCANO ─────────────────────────────────────────────
        case DisasterType::Volcano: {
            Vector2 p = position;
            float vr = volcanoRadius;

            // Rumbling jitter during pending
            if (state == DisasterState::Pending) {
                float jitter = 1.5f * std::sin((float)age * 0.6f);
                p.x += jitter;
            }

            // Volcano body (layered mountain)
            Vector2 top   = {p.x, p.y - vr * 1.2f};
            Vector2 left  = {p.x - vr * 1.4f, p.y + vr * 0.8f};
            Vector2 right = {p.x + vr * 1.4f, p.y + vr * 0.8f};
            DrawTriangle(top, left, right, {90, 50, 25, 255});

            // Lighter inner layer
            Vector2 top2  = {p.x, p.y - vr * 1.0f};
            Vector2 left2 = {p.x - vr * 0.9f, p.y + vr * 0.8f};
            Vector2 right2= {p.x + vr * 0.9f, p.y + vr * 0.8f};
            DrawTriangle(top2, left2, right2, {120, 65, 30, 255});

            // Crater opening
            DrawCircleV({p.x, p.y - vr * 1.0f}, 5.f, {50, 25, 12, 255});

            // Border lines
            DrawLineV(top, left,  {140, 80, 40, 200});
            DrawLineV(left, right, {140, 80, 40, 200});
            DrawLineV(right, top, {140, 80, 40, 200});

            if (state == DisasterState::Pending) {
                // Countdown
                int secs = timer / 60 + 1;
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%d", secs);
                int tw = MeasureText(buf, 16);
                DrawText(buf, (int)p.x - tw / 2, (int)p.y - (int)vr * 2 - 10, 16,
                         {255, 200, 50, 255});

                // Warning text
                const char* warn = "RUMBLING...";
                int ww = MeasureText(warn, 8);
                float pulse = 0.5f + 0.5f * std::sin((float)age * 0.15f);
                DrawText(warn, (int)p.x - ww / 2, (int)p.y + (int)vr + 8, 8,
                         {255, 150, 50, (unsigned char)(150 + 80 * pulse)});

                // Smoke wisps
                for (int s = 0; s < 3; s++) {
                    float sy = p.y - vr * 1.2f - (float)((age * 2 + s * 40) % 60);
                    float sx = p.x + 4.f * std::sin((float)(age + s * 30) * 0.08f);
                    unsigned char sa = (unsigned char)(60 - ((age * 2 + s * 40) % 60));
                    DrawCircleV({sx, sy}, 3.f, {100, 100, 100, sa});
                }
            }

            if (state == DisasterState::Active) {
                // Eruption glow at top
                float pulse = 0.5f + 0.5f * std::sin((float)age * 0.2f);
                DrawCircleV({p.x, p.y - vr * 1.0f}, 12.f,
                            {255, 80, 20, (unsigned char)(60 + 40 * pulse)});
                DrawCircleV({p.x, p.y - vr * 1.0f}, 7.f,
                            {255, 200, 50, (unsigned char)(100 + 50 * pulse)});

                // Eruption particles shooting upward
                for (int ep = 0; ep < 5; ep++) {
                    float ey = p.y - vr * 1.2f - (float)((age * 3 + ep * 25) % 80);
                    float ex = p.x + 8.f * std::sin((float)(age + ep * 20) * 0.12f);
                    unsigned char ea = (unsigned char)(200 - ((age * 3 + ep * 25) % 80) * 2);
                    if (ea > 200) ea = 0; // underflow guard
                    DrawCircleV({ex, ey}, 2.5f, {255, 120, 30, ea});
                }

                // Label
                const char* label = "ERUPTING!";
                int lw = MeasureText(label, 9);
                DrawText(label, (int)p.x - lw / 2, (int)p.y + (int)vr + 8, 9,
                         {255, 80, 30, (unsigned char)(180 + 60 * pulse)});

                // Draw lava puddles
                for (const auto& puddle : puddles) {
                    float lifeRatio = (float)puddle.lifetime / 300.f;
                    float lPulse = 0.5f + 0.5f * std::sin((float)age * 0.25f + puddle.position.x);

                    // Outer glow
                    DrawCircleV(puddle.position, puddle.radius * 1.3f,
                                {255, 60, 10, (unsigned char)(30 * lifeRatio)});
                    // Lava body
                    DrawCircleV(puddle.position, puddle.radius,
                                {255, 100, 20, (unsigned char)(160 * lifeRatio)});
                    // Hot center
                    DrawCircleV(puddle.position, puddle.radius * 0.5f,
                                {255, 200, 60, (unsigned char)(220 * lifeRatio * lPulse)});
                    // Border
                    DrawCircleLines((int)puddle.position.x, (int)puddle.position.y,
                                    puddle.radius, {255, 80, 20, (unsigned char)(100 * lifeRatio)});
                }
            }
            break;
        }

        // ── NUKE ────────────────────────────────────────────────
        case DisasterType::Nuke: {
            if (state == DisasterState::Pending) {
                float pulse = 0.5f + 0.5f * std::sin((float)age * 0.12f);

                // Outer danger zone
                DrawCircleV(position, outerRadius,
                            {255, 255, 50, (unsigned char)(8 + 6 * pulse)});
                DrawCircleLines((int)position.x, (int)position.y, outerRadius,
                                {255, 200, 0, (unsigned char)(50 + 40 * pulse)});

                // Inner kill zone
                DrawCircleV(position, innerRadius,
                            {255, 40, 20, (unsigned char)(12 + 8 * pulse)});
                DrawCircleLines((int)position.x, (int)position.y, innerRadius,
                                {255, 50, 20, (unsigned char)(70 + 50 * pulse)});

                // Countdown number
                int secs = timer / 60 + 1;
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%d", secs);
                int tw = MeasureText(buf, 24);
                DrawText(buf, (int)position.x - tw / 2, (int)position.y - 12, 24,
                         {255, 255, 100, 255});

                // Warning text
                const char* warn = "NUCLEAR STRIKE";
                int ww = MeasureText(warn, 10);
                DrawText(warn, (int)position.x - ww / 2, (int)position.y + 18, 10,
                         {255, 200, 50, (unsigned char)(160 + 60 * pulse)});

                // Radiation symbol at center (3 segments)
                DrawCircleV(position, 4.f, {255, 220, 0, 200});
                for (int seg = 0; seg < 3; seg++) {
                    float angle = seg * 2.094f - 1.571f; // 120° spacing
                    Vector2 segPos = {position.x + std::cos(angle) * 10.f,
                                      position.y + std::sin(angle) * 10.f};
                    DrawCircleV(segPos, 5.f, {255, 200, 0, (unsigned char)(80 + 40 * pulse)});
                }

                // Concentric warning rings (pulsing outward)
                float ringPhase = (float)(age % 60) / 60.f;
                float r1 = innerRadius * ringPhase;
                float r2 = outerRadius * ringPhase;
                DrawCircleLines((int)position.x, (int)position.y, r1,
                                {255, 200, 0, (unsigned char)(60 * (1.f - ringPhase))});
                DrawCircleLines((int)position.x, (int)position.y, r2,
                                {255, 100, 0, (unsigned char)(40 * (1.f - ringPhase))});

            } else if (state == DisasterState::Active && explosionFlash > 0) {
                float t = (float)explosionFlash / 180.f; // 1.0 → 0.0

                // Phase 1: White flash (first 30%)
                if (t > 0.7f) {
                    float flashIntensity = (t - 0.7f) / 0.3f;
                    DrawCircleV(position, outerRadius * 2.f,
                                {255, 255, 255, (unsigned char)(220 * flashIntensity)});
                }

                // Phase 2: Fireball
                float fireR = outerRadius * (1.2f - t * 0.4f);
                DrawCircleV(position, fireR,
                            {255, 80, 10, (unsigned char)(100 * t)});
                DrawCircleV(position, fireR * 0.7f,
                            {255, 160, 30, (unsigned char)(140 * t)});
                DrawCircleV(position, fireR * 0.4f,
                            {255, 240, 100, (unsigned char)(200 * t)});
                DrawCircleV(position, fireR * 0.15f,
                            {255, 255, 220, (unsigned char)(255 * t)});

                // Expanding shockwave
                float shockR = outerRadius * (2.5f - t * 1.5f);
                DrawCircleLines((int)position.x, (int)position.y, shockR,
                                {255, 200, 100, (unsigned char)(120 * t)});
                DrawCircleLines((int)position.x, (int)position.y, shockR * 0.95f,
                                {255, 150, 50, (unsigned char)(80 * t)});

                // Inner/outer zone markers (briefly visible)
                if (t > 0.5f) {
                    DrawCircleLines((int)position.x, (int)position.y, innerRadius,
                                    {255, 50, 20, (unsigned char)(100 * t)});
                    DrawCircleLines((int)position.x, (int)position.y, outerRadius,
                                    {255, 200, 0, (unsigned char)(80 * t)});
                }

                // Mushroom cloud shape (rising)
                float riseOffset = (1.f - t) * 80.f;
                float capR = 30.f + (1.f - t) * 20.f;
                DrawCircleV({position.x, position.y - riseOffset - 40.f}, capR,
                            {200, 100, 30, (unsigned char)(100 * t)});
                DrawCircleV({position.x, position.y - riseOffset - 40.f}, capR * 0.6f,
                            {255, 180, 80, (unsigned char)(120 * t)});
                // Stem
                DrawRectangle((int)(position.x - 8), (int)(position.y - riseOffset - 20),
                              16, (int)(riseOffset + 20),
                              {180, 80, 20, (unsigned char)(60 * t)});
            }
            break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  EXPIRY CHECK
// ═══════════════════════════════════════════════════════════════

bool Disaster::isExpired() const {
    return state == DisasterState::Done;
}
