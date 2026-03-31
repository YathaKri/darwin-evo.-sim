#pragma once
#include "raylib.h"
#include "Simulation.h"
#include <vector>

class UI {
public:
    static constexpr int   PANEL_X     = 860;
    static constexpr int   PANEL_W     = 180;
    static constexpr int   SCREEN_H    = 640;

    UI();

    void update(const SimStats& stats);  // call once per frame to record history
    void popLastHistory();               // remove newest graph point (for rewind)
    void draw(const SimStats& stats, PlayMode mode, int historyFrames) const;

private:
    std::vector<int> m_popHistory;       // rolling population history for graph
    static constexpr int HISTORY_LEN = 120;

    void drawStatBlock(const char* label, const char* value, int x, int& y) const;
    void drawBar(float value, float maxVal, Color barColor, int x, int y, int w) const;
    void drawGraph(int x, int y, int w, int h) const;
};
