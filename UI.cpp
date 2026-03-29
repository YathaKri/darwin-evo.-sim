#include "UI.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>

UI::UI() {
    m_popHistory.reserve(HISTORY_LEN);
}

void UI::update(const SimStats& stats) {
    m_popHistory.push_back(stats.population);
    if ((int)m_popHistory.size() > HISTORY_LEN)
        m_popHistory.erase(m_popHistory.begin());
}

void UI::drawStatBlock(const char* label, const char* value, int x, int& y) const {
    DrawText(label, x, y, 10, Color{90, 90, 120, 255});
    y += 13;
    DrawText(value, x, y, 16, Color{230, 230, 240, 255});
    y += 30;
}

void UI::drawBar(float value, float maxVal, Color barColor, int x, int y, int w) const {
    float ratio = std::clamp(value / maxVal, 0.f, 1.f);
    // Track
    DrawRectangle(x, y, w, 5, Color{30, 30, 45, 255});
    // Fill
    DrawRectangle(x, y, (int)(w * ratio), 5, barColor);
}

void UI::drawGraph(int x, int y, int w, int h) const {
    // Background
    DrawRectangle(x, y, w, h, Color{10, 10, 20, 255});
    DrawRectangleLines(x, y, w, h, Color{40, 40, 60, 255});

    if (m_popHistory.size() < 2) return;

    int maxPop = *std::max_element(m_popHistory.begin(), m_popHistory.end());
    if (maxPop == 0) maxPop = 1;

    int n = (int)m_popHistory.size();
    for (int i = 1; i < n; i++) {
        float x0 = x + (float)(i - 1) / (HISTORY_LEN - 1) * w;
        float x1 = x + (float)i       / (HISTORY_LEN - 1) * w;
        float y0 = y + h - (float)m_popHistory[i - 1] / maxPop * h;
        float y1 = y + h - (float)m_popHistory[i]     / maxPop * h;
        DrawLineEx({x0, y0}, {x1, y1}, 1.5f, Color{100, 140, 255, 200});
    }
}

void UI::draw(const SimStats& stats) const {
    int px = PANEL_X;
    int pw = PANEL_W;

    // Panel background
    DrawRectangle(px, 0, pw, SCREEN_H, Color{10, 10, 18, 245});
    DrawRectangle(px, 0, 1,  SCREEN_H, Color{45, 45, 65, 255});  // left border

    int lx = px + 12;
    int ly = 14;

    // Title
    DrawText("EVOLUTION", lx, ly, 12, Color{80, 120, 200, 255});
    ly += 16;
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 10;

    // ── Stats ──
    char buf[32];

    snprintf(buf, sizeof(buf), "%d", stats.population);
    drawStatBlock("POPULATION", buf, lx, ly);

    snprintf(buf, sizeof(buf), "%d", stats.peakPop);
    drawStatBlock("PEAK", buf, lx, ly);

    snprintf(buf, sizeof(buf), "%d", stats.foodCount);
    drawStatBlock("FOOD", buf, lx, ly);

    snprintf(buf, sizeof(buf), "%d", stats.totalBirths);
    drawStatBlock("BIRTHS", buf, lx, ly);

    // Avg size with bar
    DrawText("AVG SIZE", lx, ly, 10, Color{90, 90, 120, 255});
    ly += 13;
    snprintf(buf, sizeof(buf), "%.1f", stats.avgSize);
    DrawText(buf, lx, ly, 16, Color{230, 230, 240, 255});
    ly += 20;
    drawBar(stats.avgSize, 14.f, Color{120, 100, 255, 255}, lx, ly, pw - 24);
    ly += 18;

    // Avg speed with bar
    DrawText("AVG SPEED", lx, ly, 10, Color{90, 90, 120, 255});
    ly += 13;
    snprintf(buf, sizeof(buf), "%.1f", stats.avgSpeed);
    DrawText(buf, lx, ly, 16, Color{230, 230, 240, 255});
    ly += 20;
    drawBar(stats.avgSpeed, 8.f, Color{255, 100, 150, 255}, lx, ly, pw - 24);
    ly += 22;

    // Divider
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 10;

    // Population graph
    DrawText("POP HISTORY", lx, ly, 10, Color{90, 90, 120, 255});
    ly += 14;
    drawGraph(lx, ly, pw - 24, 70);
    ly += 78;

    // Legend
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 10;
    DrawText("LEGEND", lx, ly, 10, Color{90, 90, 120, 255});
    ly += 14;
    DrawCircleV({(float)lx + 5, (float)ly + 5}, 5, Color{100, 255, 130, 255});
    DrawText("food", lx + 14, ly, 10, Color{90, 90, 120, 255});
    ly += 16;
    DrawCircleLines(lx + 5, ly + 5, 6, Color{0, 255, 0, 200});
    DrawText("high energy", lx + 14, ly, 10, Color{90, 90, 120, 255});
    ly += 16;
    DrawCircleLines(lx + 5, ly + 5, 6, Color{255, 0, 0, 200});
    DrawText("low energy", lx + 14, ly, 10, Color{90, 90, 120, 255});
}
