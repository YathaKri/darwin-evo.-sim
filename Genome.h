#pragma once
#include "raylib.h"
#include <random>
#include <fstream>

// ── Genome: heritable genetic blueprint ──────────────────────────
// Demonstrates:
//   • operator< overloading  (fitness ranking)
//   • operator== overloading (genomic comparison)
//   • File I/O               (save / load genomes)
struct Genome {
    float         size        = 6.f;
    float         speed       = 2.4f;     // inherited speed tendency
    float         visionRange = 100.f;
    unsigned char r = 200, g = 200, b = 200;   // color channels
    int           shapeId     = 0;              // 0-4 maps to ShapeType

    // ── Fitness score (higher = fitter) ──
    float fitness() const;

    // ── Operator overloading ──
    bool operator<(const Genome& other)  const;   // compare by fitness
    bool operator==(const Genome& other) const;   // exact genomic match

    // ── Mutation (uses <random>) ──
    Genome mutate(std::mt19937& gen, float mutationRate) const;

    // ── File I/O ──
    void          save(std::ofstream& out) const;
    static Genome load(std::ifstream& in);

    // ── Helper ──
    Color getColor() const { return { r, g, b, 255 }; }
};
