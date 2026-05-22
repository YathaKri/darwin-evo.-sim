#pragma once
#include "raylib.h"
#include "Simulation.h"
#include "Creature.h"
#include <vector>
#include <string>

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
              int panelX, int screenH) const;

private:
    std::vector<int> m_popHistory;
    static constexpr int HISTORY_LEN = 120;

    void drawStatBlock(const char* label, const char* value, int x, int& y) const;
    void drawBar(float value, float maxVal, Color barColor, int x, int y, int w) const;
    void drawGraph(int x, int y, int w, int h) const;
};
