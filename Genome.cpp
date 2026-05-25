#include "Genome.h"
#include <cmath>
#include <algorithm>

// ── Fitness: composite score balancing efficiency & capability ───
float Genome::fitness() const {
    float sizeFit   = 1.0f / (size * 0.015f + 0.01f);
    float speedFit  = speed * 0.5f;
    float visionFit = visionRange * 0.01f;
    return sizeFit + speedFit + visionFit;
}

// ── operator< : fitness ranking ─────────────────────────────────
bool Genome::operator<(const Genome& other) const {
    return fitness() < other.fitness();
}

// ── operator== : exact genomic comparison ───────────────────────
bool Genome::operator==(const Genome& other) const {
    return std::abs(size - other.size)               < 0.01f &&
           std::abs(speed - other.speed)             < 0.01f &&
           std::abs(visionRange - other.visionRange) < 0.01f &&
           r == other.r && g == other.g && b == other.b &&
           shapeId == other.shapeId;
}

// ── Mutation: each trait has independent chance to mutate ────────
Genome Genome::mutate(std::mt19937& gen, float mutationRate) const {
    Genome child = *this;

    std::uniform_real_distribution<float> chance(0.f, 1.f);
    std::uniform_real_distribution<float> sizeMut(-1.f, 1.f);
    std::uniform_real_distribution<float> speedMut(-0.5f, 0.5f);
    std::uniform_real_distribution<float> visionMut(-10.f, 10.f);
    std::uniform_int_distribution<int>    colMut(-20, 20);

    if (chance(gen) < mutationRate)
        child.size = std::max(2.f, size + sizeMut(gen));

    if (chance(gen) < mutationRate)
        child.speed = std::clamp(speed + speedMut(gen), 0.5f, 5.f);

    if (chance(gen) < mutationRate)
        child.visionRange = std::clamp(visionRange + visionMut(gen), 20.f, 300.f);

    if (chance(gen) < mutationRate) {
        child.r = (unsigned char)std::clamp((int)r + colMut(gen), 30, 255);
        child.g = (unsigned char)std::clamp((int)g + colMut(gen), 30, 255);
        child.b = (unsigned char)std::clamp((int)b + colMut(gen), 30, 255);
    }

    // Shape mutation is rare (8% chance if mutation triggers)
    if (chance(gen) < mutationRate * 0.1f) {
        std::uniform_int_distribution<int> shapeDist(0, 4);
        child.shapeId = shapeDist(gen);
    }

    return child;
}

// ── File I/O: save genome to text stream ────────────────────────
void Genome::save(std::ofstream& out) const {
    out << size << " " << speed << " " << visionRange << " "
        << (int)r << " " << (int)g << " " << (int)b << " "
        << shapeId << "\n";
}

// ── File I/O: load genome from text stream ──────────────────────
Genome Genome::load(std::ifstream& in) {
    Genome gn;
    int ri, gi, bi;
    in >> gn.size >> gn.speed >> gn.visionRange >> ri >> gi >> bi >> gn.shapeId;
    gn.r = (unsigned char)ri;
    gn.g = (unsigned char)gi;
    gn.b = (unsigned char)bi;
    return gn;
}
