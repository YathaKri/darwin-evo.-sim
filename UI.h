#pragma once
#include "raylib.h"
#include "Simulation.h"
#include "Creature.h"
#include <vector>
#include <string>

// ── Info passed from main.cpp for disaster UI rendering ─────────
struct DisasterUIInfo {
    int cooldownFrames    = 0;   // frames remaining (0 = ready)
    int maxCooldown       = 600; // 10 seconds
    int inputMode         = 0;   // 0=Normal, 1=PlacingMeteor, 2=PlacingNuke
    int meteorTargetsPlaced = 0; // 0-3 for meteor placement progress
};

class UI {
public:
    static constexpr int   PANEL_W  = 180;

    UI();

    void update(const SimStats& stats);
    void popLastHistory();

    // panelX is now dynamic based on window width
    void draw(const SimStats& stats, PlayMode mode, int historyFrames,
              const Creature* selected,
              const char* statusMsg,
              int panelX, int screenH,
              const DisasterUIInfo& disasterInfo) const;

private:
    std::vector<int> m_popHistory;
    static constexpr int HISTORY_LEN = 120;

    void drawStatBlock(const char* label, const char* value, int x, int& y) const;
    void drawBar(float value, float maxVal, Color barColor, int x, int y, int w) const;
    void drawGraph(int x, int y, int w, int h) const;
};
