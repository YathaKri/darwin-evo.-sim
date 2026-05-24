#include "Simulation.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <set>

// ── Constructor ─────────────────────────────────────────────────
Simulation::Simulation() : m_gen(std::random_device{}()) {}

// ── init() ──────────────────────────────────────────────────────
void Simulation::init(int popCount, float mutationRate, int foodCount) {
    m_mutationRate   = mutationRate;
    m_currentFrame   = 0;
    m_nextCreatureId = 1;
    m_generation     = 0;
    m_totalBirths    = 0;
    m_totalFights    = 0;
    m_peakPop        = 0;

    m_population.clear();
    m_food.clear();
    m_hazards.clear();
    m_history.clear();
    m_genHistory.clear();
    m_hasGen1Stats = false;
    m_collisionCooldowns.clear();
    while (!m_events.empty()) m_events.pop();

    std::uniform_real_distribution<float> posX(50.f, std::max(51.f, m_worldW - 50.f));
    std::uniform_real_distribution<float> posY(50.f, std::max(51.f, m_worldH - 50.f));
    std::uniform_real_distribution<float> velDist(-2.f, 2.f);
    std::uniform_int_distribution<int>    colorDist(50, 255);
    std::uniform_real_distribution<float> sizeDist(4.f, 10.f);
    std::uniform_real_distribution<float> speedDist(1.2f, 3.6f);
    std::uniform_int_distribution<int>    shapeDist(0, 4);

    for (int i = 0; i < popCount; i++) {
        auto c = std::make_unique<Creature>();
        c->genome.size        = sizeDist(m_gen);
        c->genome.speed       = speedDist(m_gen);
        c->genome.visionRange = 100.f;
        c->genome.r           = (unsigned char)colorDist(m_gen);
        c->genome.g           = (unsigned char)colorDist(m_gen);
        c->genome.b           = (unsigned char)colorDist(m_gen);
        c->genome.shapeId     = shapeDist(m_gen);
        c->shape              = (ShapeType)c->genome.shapeId;
        c->applyGenome();
        c->position     = {posX(m_gen), posY(m_gen)};
        c->velocity     = {velDist(m_gen), velDist(m_gen)};
        c->id           = m_nextCreatureId++;
        c->mutationRate = mutationRate;
        m_population.push_back(std::move(c));
    }

    for (int i = 0; i < foodCount; i++)
        m_food.push_back(Food::spawn(m_gen, m_worldW, m_worldH));

    std::uniform_int_distribution<int> hazardDelay(300, 600);
    m_events.push({hazardDelay(m_gen), SimEvent::SpawnHazard});
    m_events.push({300,                SimEvent::GenerationTick});
}

// ═══════════════════════════════════════════════════════════════
//  COLLISION COOLDOWN
// ═══════════════════════════════════════════════════════════════

// Szudzik pairing: collision-free for all non-negative integers
static long long szudzikPair(int a, int b) {
    long long lo = std::min(a, b);
    long long hi = std::max(a, b);
    return hi * hi + hi + lo;
}

bool Simulation::isOnCooldown(int idA, int idB) const {
    long long key = szudzikPair(idA, idB);
    for (const auto& p : m_collisionCooldowns)
        if (p.first == key && p.second > m_currentFrame) return true;
    return false;
}

void Simulation::setCooldown(int idA, int idB, int frames) {
    long long key = szudzikPair(idA, idB);
    // Update existing or add new
    for (auto& p : m_collisionCooldowns) {
        if (p.first == key) { p.second = m_currentFrame + frames; return; }
    }
    m_collisionCooldowns.push_back({key, m_currentFrame + frames});
}

// ═══════════════════════════════════════════════════════════════
//  SNAPSHOT / REWIND
// ═══════════════════════════════════════════════════════════════

void Simulation::saveSnapshot() {
    if ((int)m_history.size() >= MAX_HISTORY)
        m_history.pop_front();

    Snapshot s;
    s.population.reserve(m_population.size());
    for (const auto& c : m_population)
        s.population.push_back(*c);

    s.food = m_food;

    s.hazards.reserve(m_hazards.size());
    for (const auto& h : m_hazards)
        s.hazards.push_back(*h);

    s.totalBirths    = m_totalBirths;
    s.totalFights    = m_totalFights;
    s.peakPop        = m_peakPop;
    s.currentFrame   = m_currentFrame;
    s.nextCreatureId = m_nextCreatureId;
    s.generation     = m_generation;

    // Save disasters and cooldowns for complete rewind
    s.disasters          = m_disasters;
    s.collisionCooldowns = m_collisionCooldowns;

    m_history.push_back(std::move(s));
}

void Simulation::rewindOneStep() {
    if (m_history.empty()) return;

    Snapshot& snap = m_history.back();

    m_population.clear();
    for (auto& c : snap.population)
        m_population.push_back(std::make_unique<Creature>(std::move(c)));

    m_food = std::move(snap.food);

    m_hazards.clear();
    for (auto& h : snap.hazards)
        m_hazards.push_back(std::make_unique<Hazard>(std::move(h)));

    m_totalBirths    = snap.totalBirths;
    m_totalFights    = snap.totalFights;
    m_peakPop        = snap.peakPop;
    m_currentFrame   = snap.currentFrame;
    m_nextCreatureId = snap.nextCreatureId;
    m_generation     = snap.generation;

    // Restore disasters and cooldowns
    m_disasters          = std::move(snap.disasters);
    m_collisionCooldowns = std::move(snap.collisionCooldowns);

    // Clear and re-seed event queue based on restored frame
    while (!m_events.empty()) m_events.pop();
    std::uniform_int_distribution<int> hazardDelay(400, 800);
    m_events.push({m_currentFrame + hazardDelay(m_gen), SimEvent::SpawnHazard});
    m_events.push({m_currentFrame + 300,                SimEvent::GenerationTick});

    m_history.pop_back();
}

// ═══════════════════════════════════════════════════════════════
//  EVENT PROCESSING
// ═══════════════════════════════════════════════════════════════

void Simulation::processEvent(const SimEvent& ev) {
    switch (ev.type) {
        case SimEvent::SpawnHazard: {
            if ((int)m_hazards.size() < 3) {
                m_hazards.push_back(
                    std::make_unique<Hazard>(Hazard::spawn(m_gen, m_worldW, m_worldH)));
            }
            std::uniform_int_distribution<int> nextDist(400, 800);
            m_events.push({m_currentFrame + nextDist(m_gen), SimEvent::SpawnHazard});
            break;
        }
        case SimEvent::GenerationTick: {
            recordGeneration();
            m_events.push({m_currentFrame + 300, SimEvent::GenerationTick});
            break;
        }
    }
}

void Simulation::recordGeneration() {
    SimStats s = getStats();
    GenStats gs;
    gs.generation = m_generation++;
    gs.avgSize    = s.avgSize;
    gs.avgSpeed   = s.avgSpeed;
    gs.avgVision  = s.avgVision;
    gs.population = s.population;
    m_genHistory.push_back(gs);

    if (!m_hasGen1Stats) {
        m_gen1Stats = gs;
        m_hasGen1Stats = true;
    }

    if ((int)m_genHistory.size() > 50)
        m_genHistory.erase(m_genHistory.begin());
}

// ═══════════════════════════════════════════════════════════════
//  MAIN UPDATE LOOP
// ═══════════════════════════════════════════════════════════════

void Simulation::update() {
    saveSnapshot();
    ++m_currentFrame;

    // ── Clean expired cooldowns periodically ────────────────────
    if (m_currentFrame % 60 == 0) {
        m_collisionCooldowns.erase(
            std::remove_if(m_collisionCooldowns.begin(), m_collisionCooldowns.end(),
                [&](const std::pair<long long, int>& p) { return p.second <= m_currentFrame; }),
            m_collisionCooldowns.end());
    }

    // ── Process scheduled events ────────────────────────────────
    while (!m_events.empty() && m_events.top().triggerFrame <= m_currentFrame) {
        SimEvent ev = m_events.top();
        m_events.pop();
        processEvent(ev);
    }

    // ── Food regrowth ───────────────────────────────────────────
    std::uniform_int_distribution<int> chance(1, 100);
    for (int i = 0; i < m_foodDropMult; i++) {
        if (chance(m_gen) <= 10 && (int)m_food.size() < 200 * m_foodDropMult) {
            m_food.push_back(Food::spawn(m_gen, m_worldW, m_worldH));
        }
    }

    // ── Remove food inside Famine hazards ───────────────────────
    m_food.erase(std::remove_if(m_food.begin(), m_food.end(), [&](const Food& f) {
        for (const auto& h : m_hazards) {
            if (h->getType() == HazardType::Famine) {
                float dx = f.position.x - h->getPosition().x;
                float dy = f.position.y - h->getPosition().y;
                if (dx*dx + dy*dy < h->getRadius() * h->getRadius()) return true;
            }
        }
        return false;
    }), m_food.end());

    // ── Validate and pair up mates ──────────────────────────────
    // Clean dead mate targets
    for (auto& c : m_population) {
        if (c->mateTargetId != -1) {
            if (!findCreature(c->mateTargetId)) c->mateTargetId = -1;
        }
    }
    for (auto& c : m_population) {
        if (c->energy < 160.f) {
            c->mateTargetId = -1;
        } else if (c->mateTargetId != -1) {
            bool found = false;
            for (auto& other : m_population) {
                if (other->id == c->mateTargetId && other->energy >= 160.f) {
                    found = true;
                    break;
                }
            }
            if (!found) c->mateTargetId = -1;
        }
    }

    for (auto& c : m_population) {
        if (c->energy >= 160.f && c->mateTargetId == -1) {
            for (auto& other : m_population) {
                if (other->id != c->id && other->energy >= 160.f && other->mateTargetId == -1) {
                    float dx = other->position.x - c->position.x;
                    float dy = other->position.y - c->position.y;
                    float d = std::sqrt(dx*dx + dy*dy);
                    if (d < c->visionRange || d < other->visionRange) {
                        c->mateTargetId = other->id;
                        other->mateTargetId = c->id;
                        break;
                    }
                }
            }
        }
    }

    // ── Per-creature update ─────────────────────────────────────
    std::vector<std::unique_ptr<Creature>> newBabies;
    std::vector<int> deadIds;   // creatures killed in fights
    std::set<int> eatenFood;    // food indices eaten this frame (deferred removal)

    for (auto& c : m_population) {
        c->update(m_worldW, m_worldH, m_food, m_population, m_hazards, m_gen);

        // ── Hazard interaction ──────────────────────────────
        bool inAnyHazard = false;
        for (const auto& h : m_hazards) {
            float dx   = c->position.x - h->getPosition().x;
            float dy   = c->position.y - h->getPosition().y;
            float dist = std::sqrt(dx * dx + dy * dy);
            float outerR = h->getRadius();
            float innerR = outerR * 0.7f;

            bool isImmune = (h->getType() == HazardType::Toxic && c->toxImmune) ||
                            (h->getType() == HazardType::Radiation && c->radImmune);

            if (dist < outerR) {
                if (!isImmune && h->getType() != HazardType::Famine) {
                    // Take damage
                    c->hp -= h->getInstantDamage();
                    c->flashTimer = 8;

                    if (h->getType() == HazardType::Toxic) {
                        c->dotDamage = h->getDotDamage();
                        c->dotTimer  = h->getDotDuration();
                    }
                }

                // Step 1: If inside inner ring, FLEE to nearest exit
                if (dist < innerR && !isImmune && h->getType() != HazardType::Famine) {
                    inAnyHazard = true;
                    c->fleeing = true;
                    // Flee target: point on outer edge in the direction away from center
                    if (dist > 0.001f) {
                        c->fleeTarget.x = h->getPosition().x + (dx / dist) * (outerR + 10.f);
                        c->fleeTarget.y = h->getPosition().y + (dy / dist) * (outerR + 10.f);
                    } else {
                        // Directly on center — pick random direction
                        c->fleeTarget.x = h->getPosition().x + outerR + 10.f;
                        c->fleeTarget.y = h->getPosition().y;
                    }
                }
            } else {
                // Step 2: Just exited a hazard? Grant immunity as a survivor
                // Check if creature was recently damaged by this type
                if (dist < outerR + 15.f && dist > outerR) {
                    if (h->getType() == HazardType::Toxic && !c->toxImmune && c->hp > 0 && c->hp < c->maxHp * 0.95f) {
                        c->toxImmune = true;
                    }
                    if (h->getType() == HazardType::Radiation && !c->radImmune && c->hp > 0 && c->hp < c->maxHp * 0.95f) {
                        c->radImmune = true;
                    }
                    if (h->getType() == HazardType::Famine && !c->famBadge) {
                        c->famBadge = true;
                    }
                }
            }
        }

        // Clear flee flag if no longer inside any hazard inner ring
        if (!inAnyHazard && c->fleeing) {
            c->fleeing = false;
        }

        // ── Eat food (collect indices, erase after loop) ────────
        for (int fi = 0; fi < (int)m_food.size(); ++fi) {
            float dx   = c->position.x - m_food[fi].position.x;
            float dy   = c->position.y - m_food[fi].position.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < c->radius + m_food[fi].radius) {
                if (m_food[fi].isBig) {
                    c->bigFoodTimer = 300; // 5 seconds of 100% full food buff
                    c->energy = 200.f;     // instantly fill up to max
                } else {
                    // (c) Stamina mutation: +5% extra energy gain per stack (max 50%)
                    float gainMult = 1.f + (c->mutations.staminaCount * 0.05f);
                    if (gainMult > 1.5f) gainMult = 1.5f;
                    c->energy += 40.f * gainMult;
                }
                eatenFood.insert(fi);
            }
        }
    }

    // ── Creature-Creature collisions: FIGHT or REPRODUCE ────────
    int n = (int)m_population.size();
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            auto& a = m_population[i];
            auto& b = m_population[j];

            // Skip if either is already dead this frame
            if (a->hp <= 0 || b->hp <= 0) continue;
            if (a->energy <= 0 || b->energy <= 0) continue;

            float dx   = a->position.x - b->position.x;
            float dy   = a->position.y - b->position.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            float touchDist = a->radius + b->radius;

            if (dist < touchDist) {
                // ── Physical Repulsion (Soft Collision) ─────────
                if (dist > 0.001f) {
                    float overlap = touchDist - dist;
                    float pushFactor = overlap * 0.05f;
                    float pushX = (dx / dist) * pushFactor;
                    float pushY = (dy / dist) * pushFactor;
                    a->position.x += pushX;
                    a->position.y += pushY;
                    b->position.x -= pushX;
                    b->position.y -= pushY;
                }

                // Check cooldown to prevent spamming
                if (isOnCooldown(a->id, b->id)) continue;
                setCooldown(a->id, b->id, 120);  // 2 second cooldown

                // ── Decide: fight or reproduce? ─────────────────
                // Similar colors = more likely to reproduce (same "species")
                int colorDiff = std::abs((int)a->genome.r - (int)b->genome.r) +
                                std::abs((int)a->genome.g - (int)b->genome.g) +
                                std::abs((int)a->genome.b - (int)b->genome.b);

                // Both need enough energy to reproduce
                bool canReproduce = (a->energy >= 120.f && b->energy >= 120.f);
                bool superHighEnergy = (a->energy >= 160.f && b->energy >= 160.f); // 80% of 200

                // Step 3: Badge holders (immune organisms) reproduce at 40% energy
                bool badgeReproduce = (a->hasBadge() || b->hasBadge()) &&
                                      (a->energy >= 80.f && b->energy >= 80.f);

                // Check 5-second reproduction cooldown (300 frames)
                if (a->reproductionCooldown > 0 || b->reproductionCooldown > 0) {
                    canReproduce = false;
                    superHighEnergy = false;
                    badgeReproduce = false;
                }

                // 100% chance to reproduce if >80% energy, ignoring species color gap
                if (superHighEnergy || badgeReproduce || (canReproduce && colorDiff < 200)) {
                    // ── REPRODUCE ───────────────────────────────
                    Creature baby = a->reproduce(m_gen, *b);
                    baby.id = m_nextCreatureId++;
                    
                    setCooldown(a->id, b->id, 120);
                    setCooldown(a->id, baby.id, 120);
                    setCooldown(b->id, baby.id, 120);

                    // Set 5 second reproduction cooldown (300 frames)
                    a->reproductionCooldown = 300;
                    b->reproductionCooldown = 300;

                    newBabies.push_back(std::make_unique<Creature>(std::move(baby)));
                    a->energy -= 50.f;
                    b->energy -= 50.f;
                    a->mateTargetId = -1;
                    b->mateTargetId = -1;
                    ++m_totalBirths;
                } else {
                    // ── FIGHT ───────────────────────────────────
                    // Immune if either is seeking a mate
                    if (a->mateTargetId != -1 || b->mateTargetId != -1) {
                        continue;
                    }

                    setCooldown(a->id, b->id, 60);

                    float powerA = a->getCombatPower();
                    float powerB = b->getCombatPower();

                    // Add some randomness to fight outcomes
                    std::uniform_real_distribution<float> fightRng(0.8f, 1.2f);
                    powerA *= fightRng(m_gen);
                    powerB *= fightRng(m_gen);

                    float damage = 20.f + std::abs(powerA - powerB) * 0.5f;

                    if (powerA >= powerB) {
                        b->hp -= damage;
                        b->flashTimer = 8;
                        a->energy += 15.f;  // winner gains some energy
                    } else {
                        a->hp -= damage;
                        a->flashTimer = 8;
                        b->energy += 15.f;
                    }
                    ++m_totalFights;
                }
            }
        }
    }

    for (auto& baby : newBabies)
        m_population.push_back(std::move(baby));

    // ── Deferred food removal (erase from back to preserve indices) ──
    for (auto it = eatenFood.rbegin(); it != eatenFood.rend(); ++it) {
        if (*it < (int)m_food.size()) {
            m_food.erase(m_food.begin() + *it);
        }
    }

    // ── Update & cull hazards ───────────────────────────────────
    for (auto& h : m_hazards)
        h->update();

    m_hazards.erase(
        std::remove_if(m_hazards.begin(), m_hazards.end(),
            [](const std::unique_ptr<Hazard>& h) { return h->isExpired(); }),
        m_hazards.end());

    // ── Update input disasters ─────────────────────────────────
    if (m_disasterCooldown > 0) --m_disasterCooldown;

    for (auto& d : m_disasters) {
        d.update(m_gen, m_worldW, m_worldH);
        d.applyDamage(m_population);
    }

    m_disasters.erase(
        std::remove_if(m_disasters.begin(), m_disasters.end(),
            [](const Disaster& d) { return d.isExpired(); }),
        m_disasters.end());

    // ── Death: energy depleted OR hp depleted ───────────────────
    m_population.erase(
        std::remove_if(m_population.begin(), m_population.end(),
            [](const std::unique_ptr<Creature>& c) {
                return c->energy <= 0 || c->hp <= 0;
            }),
        m_population.end());

    if ((int)m_population.size() > m_peakPop)
        m_peakPop = (int)m_population.size();
}

// ═══════════════════════════════════════════════════════════════
//  DRAW
// ═══════════════════════════════════════════════════════════════

void Simulation::draw() const {
    for (const auto& f : m_food)       f.draw();
    for (const auto& h : m_hazards)    h->draw();
    for (const auto& c : m_population) c->draw();
    for (const auto& d : m_disasters)  d.draw();
}

void Simulation::spawnDisaster(Disaster d) {
    m_disasters.push_back(std::move(d));
    m_disasterCooldown = Disaster::COOLDOWN_FRAMES;
}

// ═══════════════════════════════════════════════════════════════
//  STATS
// ═══════════════════════════════════════════════════════════════

SimStats Simulation::getStats() const {
    SimStats s;
    s.population  = (int)m_population.size();
    s.peakPop     = m_peakPop;
    s.foodCount   = (int)m_food.size();
    s.totalBirths = m_totalBirths;
    s.totalFights = m_totalFights;
    s.hazardCount = (int)m_hazards.size();
    s.generation  = m_generation;
    s.simSpeedMult = m_simSpeedMult;
    s.foodDropMult = m_foodDropMult;

    if (!m_population.empty()) {
        for (const auto& c : m_population) {
            s.avgSize   += c->radius;
            s.avgSpeed  += std::sqrt(c->velocity.x * c->velocity.x +
                                     c->velocity.y * c->velocity.y);
            s.avgVision += c->visionRange;
        }
        s.avgSize   /= (float)m_population.size();
        s.avgSpeed  /= (float)m_population.size();
        s.avgVision /= (float)m_population.size();
    }

    if (!m_genHistory.empty()) {
        s.hasPrevGen    = true;
        const GenStats& prev = m_genHistory.back();
        s.prevAvgSize   = prev.avgSize;
        s.prevAvgSpeed  = prev.avgSpeed;
        s.prevAvgVision = prev.avgVision;
    }

    if (m_hasGen1Stats) {
        s.hasGen1       = true;
        s.gen1AvgSize   = m_gen1Stats.avgSize;
        s.gen1AvgSpeed  = m_gen1Stats.avgSpeed;
        s.gen1AvgVision = m_gen1Stats.avgVision;
    }

    return s;
}

// ═══════════════════════════════════════════════════════════════
//  FILE I/O
// ═══════════════════════════════════════════════════════════════

void Simulation::saveGenomes(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << m_population.size() << "\n";
    for (const auto& c : m_population)
        c->genome.save(file);

    file.close();
}

void Simulation::loadGenomes(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    int count;
    file >> count;

    m_population.clear();
    std::uniform_real_distribution<float> posX(50.f, std::max(51.f, m_worldW - 50.f));
    std::uniform_real_distribution<float> posY(50.f, std::max(51.f, m_worldH - 50.f));

    for (int i = 0; i < count && file.good(); i++) {
        Genome gn  = Genome::load(file);
        auto   c   = std::make_unique<Creature>();
        c->genome  = gn;
        c->shape   = (ShapeType)gn.shapeId;
        c->applyGenome();
        c->position = {posX(m_gen), posY(m_gen)};
        float angle = std::uniform_real_distribution<float>(0.f, 6.28318f)(m_gen);
        c->velocity = {gn.speed * std::cos(angle), gn.speed * std::sin(angle)};
        c->id           = m_nextCreatureId++;
        c->mutationRate = m_mutationRate;
        c->energy       = 100.f;
        m_population.push_back(std::move(c));
    }

    file.close();
}

void Simulation::generateReport(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    SimStats s = getStats();

    file << "====================================\n";
    file << "  DARWIN EVOLUTION SIM - REPORT\n";
    file << "====================================\n\n";

    file << "Population:     " << s.population  << "\n";
    file << "Peak:           " << s.peakPop     << "\n";
    file << "Total Births:   " << s.totalBirths << "\n";
    file << "Total Fights:   " << s.totalFights << "\n";
    file << "Food:           " << s.foodCount   << "\n";
    file << "Hazards:        " << s.hazardCount << "\n";
    file << "Generation:     " << s.generation  << "\n";
    file << "Frame:          " << m_currentFrame << "\n\n";

    char buf[128];
    file << "-- Averages --\n";
    std::snprintf(buf, sizeof(buf), "Avg Size:   %.2f\n", s.avgSize);   file << buf;
    std::snprintf(buf, sizeof(buf), "Avg Speed:  %.2f\n", s.avgSpeed);  file << buf;
    std::snprintf(buf, sizeof(buf), "Avg Vision: %.2f\n", s.avgVision); file << buf;
    file << "\n";

    if (!m_genHistory.empty()) {
        file << "-- Generation History --\n";
        file << "Gen  | Pop  | Size   | Speed  | Vision\n";
        file << "-----+------+--------+--------+-------\n";
        for (const auto& g : m_genHistory) {
            std::snprintf(buf, sizeof(buf), "%-4d | %-4d | %6.2f | %6.2f | %6.2f\n",
                     g.generation, g.population, g.avgSize, g.avgSpeed, g.avgVision);
            file << buf;
        }
        file << "\n";
    }

    file << "-- Creatures (sorted by fitness) --\n";
    std::vector<const Creature*> sorted;
    for (const auto& c : m_population) sorted.push_back(c.get());
    std::sort(sorted.begin(), sorted.end(),
        [](const Creature* a, const Creature* b) {
            return b->genome < a->genome;
        });

    file << "ID   | Age  | HP     | Energy | Size  | Speed | Vision | Fitness\n";
    file << "-----+------+--------+--------+-------+-------+--------+--------\n";
    for (const auto* c : sorted) {
        float spd = std::sqrt(c->velocity.x * c->velocity.x +
                              c->velocity.y * c->velocity.y);
        std::snprintf(buf, sizeof(buf),
                 "%-4d | %-4d | %6.1f | %6.1f | %5.1f | %5.1f | %6.1f | %7.2f\n",
                 c->id, c->age, c->hp, c->energy, c->radius, spd,
                 c->visionRange, c->genome.fitness());
        file << buf;
    }

    file.close();
}

// ═══════════════════════════════════════════════════════════════
//  SEARCH
// ═══════════════════════════════════════════════════════════════

const Creature* Simulation::findCreature(int id) const {
    for (const auto& c : m_population)
        if (c->id == id) return c.get();
    return nullptr;
}
