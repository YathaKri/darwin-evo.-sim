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

void UI::popLastHistory() {
    if (!m_popHistory.empty())
        m_popHistory.pop_back();
}

// ── Helpers ─────────────────────────────────────────────────────

void UI::drawStatBlock(const char* label, const char* value, int x, int& y) const {
    DrawText(label, x, y, 8, Color{90, 90, 120, 255});
    y += 10;
    DrawText(value, x, y, 12, Color{230, 230, 240, 255});
    y += 18;
}

void UI::drawBar(float value, float maxVal, Color barColor, int x, int y, int w) const {
    float ratio = std::clamp(value / maxVal, 0.f, 1.f);
    DrawRectangle(x, y, w, 4, Color{30, 30, 45, 255});
    DrawRectangle(x, y, (int)(w * ratio), 4, barColor);
}

void UI::drawGraph(int x, int y, int w, int h) const {
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

// ═══════════════════════════════════════════════════════════════
//  MAIN DRAW
// ═══════════════════════════════════════════════════════════════

void UI::draw(const SimStats& stats, PlayMode mode, int historyFrames,
              const Creature* selected,
              const char* statusMsg,
              int panelX, int screenH) const {
    int px = panelX;
    int pw = PANEL_W;

    // ── Panel background ────────────────────────────────────────
    DrawRectangle(px, 0, pw, screenH, Color{10, 10, 18, 245});
    DrawRectangle(px, 0, 1,  screenH, Color{45, 45, 65, 255});

    int lx = px + 12;
    int ly = 10;

    // ── Title ───────────────────────────────────────────────────
    DrawText("EVOLUTION", lx, ly, 12, Color{80, 120, 200, 255});
    ly += 16;
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 8;

    // ── Playback state ──────────────────────────────────────────
    {
        const char* label = "";
        Color       col   = {230, 230, 240, 255};

        switch (mode) {
            case PlayMode::Playing:     label = ">> PLAYING";    col = {80, 255, 130, 255};  break;
            case PlayMode::Paused:      label = "|| PAUSED";     col = {255, 200, 80, 255};  break;
            case PlayMode::FastForward: label = ">>> FAST FWD";  col = {100, 180, 255, 255}; break;
            case PlayMode::Rewinding:   label = "<< REWIND";     col = {255, 100, 130, 255}; break;
        }

        DrawText(label, lx, ly, 10, col);
        ly += 14;

        // Rewind buffer bar
        DrawText("REWIND BUFFER", lx, ly, 7, Color{70, 70, 100, 255});
        ly += 9;
        float bufRatio = std::clamp((float)historyFrames / 600.f, 0.f, 1.f);
        DrawRectangle(lx, ly, pw - 24, 4, Color{30, 30, 45, 255});
        Color barCol = (mode == PlayMode::Rewinding)
                           ? Color{255, 100, 130, 200}
                           : Color{80, 80, 120, 200};
        DrawRectangle(lx, ly, (int)((pw - 24) * bufRatio), 4, barCol);
        ly += 8;

        DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
        ly += 8;
    }

    // ── Core stats ──────────────────────────────────────────────
    char buf[64];

    std::snprintf(buf, sizeof(buf), "%d", stats.population);
    drawStatBlock("POPULATION", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.peakPop);
    drawStatBlock("PEAK", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.foodCount);
    drawStatBlock("FOOD", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.totalBirths);
    drawStatBlock("BIRTHS", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.totalFights);
    drawStatBlock("FIGHTS", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.hazardCount);
    drawStatBlock("HAZARDS", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.generation);
    drawStatBlock("GEN", buf, lx, ly);

    // ── Avg traits with generation comparison ───────────────────
    auto drawAvgStat = [&](const char* name, float val, float prevVal,
                           bool hasPrev, float barMax, Color bCol) {
        DrawText(name, lx, ly, 8, Color{90, 90, 120, 255});
        ly += 10;

        std::snprintf(buf, sizeof(buf), "%.1f", val);
        DrawText(buf, lx, ly, 12, Color{230, 230, 240, 255});

        // Trend delta
        if (hasPrev) {
            float delta = val - prevVal;
            char deltaBuf[32];
            std::snprintf(deltaBuf, sizeof(deltaBuf), "%+.1f", delta);
            Color dc = (delta >= 0) ? Color{80, 255, 130, 200} : Color{255, 80, 80, 200};
            DrawText(deltaBuf, lx + 50, ly + 2, 8, dc);
        }
        ly += 14;

        drawBar(val, barMax, bCol, lx, ly, pw - 24);
        ly += 10;
    };

    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 6;

    drawAvgStat("AVG SIZE",   stats.avgSize,   stats.prevAvgSize,
                stats.hasPrevGen, 14.f,  Color{120, 100, 255, 255});
    drawAvgStat("AVG SPEED",  stats.avgSpeed,  stats.prevAvgSpeed,
                stats.hasPrevGen, 8.f,   Color{255, 100, 150, 255});
    drawAvgStat("AVG VISION", stats.avgVision, stats.prevAvgVision,
                stats.hasPrevGen, 300.f, Color{255, 200, 80, 255});

    // ── Population graph ────────────────────────────────────────
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 6;
    DrawText("POP HISTORY", lx, ly, 8, Color{90, 90, 120, 255});
    ly += 10;
    drawGraph(lx, ly, pw - 24, 50);
    ly += 54;

    // ── Selected organism info ──────────────────────────────────
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 6;

    if (selected) {
        DrawText("SELECTED", lx, ly, 8, Color{0, 255, 120, 255});
        ly += 12;

        // ID
        std::snprintf(buf, sizeof(buf), "ID: %d", selected->id);
        DrawText(buf, lx, ly, 10, Color{230, 230, 240, 255});
        ly += 14;

        // HP bar
        DrawText("HP", lx, ly, 7, Color{90, 90, 120, 255});
        ly += 9;
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", selected->hp, selected->maxHp);
        DrawText(buf, lx, ly, 10, Color{230, 230, 240, 255});
        ly += 12;
        float hpRatio = std::clamp(selected->hp / selected->maxHp, 0.f, 1.f);
        Color hpCol = (hpRatio > 0.5f)  ? Color{80, 255, 100, 220}
                     : (hpRatio > 0.25f) ? Color{255, 200, 60, 220}
                     :                     Color{255, 60, 60, 220};
        drawBar(selected->hp, selected->maxHp, hpCol, lx, ly, pw - 24);
        ly += 8;

        // Energy bar
        DrawText("ENERGY", lx, ly, 7, Color{90, 90, 120, 255});
        ly += 9;
        std::snprintf(buf, sizeof(buf), "%.0f", selected->energy);
        DrawText(buf, lx, ly, 10, Color{230, 230, 240, 255});
        ly += 12;
        drawBar(selected->energy, 200.f, Color{255, 200, 80, 220}, lx, ly, pw - 24);
        ly += 8;

        // Age
        std::snprintf(buf, sizeof(buf), "Age: %d", selected->age);
        DrawText(buf, lx, ly, 9, Color{180, 180, 200, 255});
        ly += 12;

        // Size / Speed / Vision
        float spd = std::sqrt(selected->velocity.x * selected->velocity.x +
                              selected->velocity.y * selected->velocity.y);
        std::snprintf(buf, sizeof(buf), "Size: %.1f", selected->radius);
        DrawText(buf, lx, ly, 9, Color{180, 180, 200, 255});
        ly += 12;
        std::snprintf(buf, sizeof(buf), "Speed: %.1f", spd);
        DrawText(buf, lx, ly, 9, Color{180, 180, 200, 255});
        ly += 12;
        std::snprintf(buf, sizeof(buf), "Vision: %.0f", selected->visionRange);
        DrawText(buf, lx, ly, 9, Color{180, 180, 200, 255});
        ly += 12;

        // Fitness
        std::snprintf(buf, sizeof(buf), "Fitness: %.1f", selected->genome.fitness());
        DrawText(buf, lx, ly, 9, Color{100, 200, 255, 255});
        ly += 12;

        // Shape
        const char* shapeNames[] = {"Circle", "Triangle", "Diamond", "Pentagon", "Hexagon"};
        int si = (int)selected->shape;
        if (si >= 0 && si <= 4)
            std::snprintf(buf, sizeof(buf), "Shape: %s", shapeNames[si]);
        else
            std::snprintf(buf, sizeof(buf), "Shape: ???");
        DrawText(buf, lx, ly, 9, Color{180, 180, 200, 255});
        ly += 12;

        // DOT status
        if (selected->dotTimer > 0) {
            std::snprintf(buf, sizeof(buf), "POISONED (%.0fs)", (float)selected->dotTimer / 60.f);
            DrawText(buf, lx, ly, 9, Color{160, 40, 255, 255});
            ly += 12;
        }

        // Mating status
        if (selected->mateTargetId != -1) {
            std::snprintf(buf, sizeof(buf), "Seeking mate #%d", selected->mateTargetId);
            DrawText(buf, lx, ly, 9, Color{255, 105, 180, 255});
            ly += 12;
        }

        // Flee status
        if (selected->fleeing) {
            DrawText("FLEEING HAZARD!", lx, ly, 9, Color{255, 80, 80, 255});
            ly += 12;
        }

        // Immunity badges
        if (selected->toxImmune || selected->radImmune) {
            DrawText("BADGES:", lx, ly, 7, Color{90, 90, 120, 255});
            ly += 10;
            if (selected->toxImmune) {
                DrawText("\xe2\x97\x86 TOX Immune (Silver)", lx, ly, 9, Color{192, 192, 210, 255});
                ly += 12;
            }
            if (selected->radImmune) {
                DrawText("\xe2\x98\x85 RAD Immune (Gold)", lx, ly, 9, Color{255, 215, 0, 255});
                ly += 12;
            }
        }

        // Mutation stack display
        if (selected->mutations.hasMutations()) {
            DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
            ly += 4;
            DrawText("MUTATIONS", lx, ly, 8, Color{90, 90, 120, 255});
            ly += 10;

            if (selected->mutations.speedCount > 0) {
                std::snprintf(buf, sizeof(buf), "Speed x%d (+%.0f%%)",
                         selected->mutations.speedCount, selected->mutations.speedBonus);
                DrawText(buf, lx, ly, 8, Color{100, 200, 255, 255});
                ly += 10;
            }
            if (selected->mutations.visionCount > 0) {
                std::snprintf(buf, sizeof(buf), "Vision x%d (+%.0f%%)",
                         selected->mutations.visionCount, selected->mutations.visionBonus);
                DrawText(buf, lx, ly, 8, Color{255, 220, 100, 255});
                ly += 10;
            }
            if (selected->mutations.staminaCount > 0) {
                std::snprintf(buf, sizeof(buf), "Stamina x%d (-%d%% cost)",
                         selected->mutations.staminaCount, selected->mutations.staminaCount * 5);
                DrawText(buf, lx, ly, 8, Color{255, 255, 80, 255});
                ly += 10;
            }
            if (selected->mutations.sizeCount > 0) {
                std::snprintf(buf, sizeof(buf), "Size x%d (+%d%% body)",
                         selected->mutations.sizeCount, selected->mutations.sizeCount * 10);
                DrawText(buf, lx, ly, 8, Color{200, 120, 255, 255});
                ly += 10;
            }
            if (selected->mutations.hasBio) {
                std::snprintf(buf, sizeof(buf), "Biolum. x%d", selected->mutations.bioCount);
                DrawText(buf, lx, ly, 8, selected->mutations.neonColor);
                ly += 10;
            }
        }

        ly += 2;
    } else {
        DrawText("INSPECT", lx, ly, 8, Color{90, 90, 120, 255});
        ly += 12;
        DrawText("Click an organism", lx, ly, 8, Color{60, 60, 90, 255});
        ly += 10;
        DrawText("Right-click to deselect", lx, ly, 7, Color{50, 50, 75, 255});
        ly += 12;
    }

    // ── Controls ────────────────────────────────────────────────
    ly = screenH - 130;
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 6;

    // Sim Speed
    DrawText("SIM SPEED", lx, ly, 8, Color{90, 90, 120, 255});
    ly += 12;
    int btns[] = {1, 5, 10, 15, 20};
    for (int i = 0; i < 5; i++) {
        int bx = lx + i * 28;
        Color bCol = (stats.simSpeedMult == btns[i]) ? Color{100, 200, 255, 255} : Color{50, 50, 70, 255};
        DrawRectangle(bx, ly, 24, 14, bCol);
        std::snprintf(buf, sizeof(buf), "%dx", btns[i]);
        DrawText(buf, bx + 2, ly + 2, 8, (stats.simSpeedMult == btns[i]) ? Color{0, 0, 0, 255} : Color{200, 200, 200, 255});
    }
    ly += 20;

    // Food Drop Rate
    DrawText("FOOD DROP RATE", lx, ly, 8, Color{90, 90, 120, 255});
    ly += 12;
    int fBtns[] = {1, 2, 5, 10, 20};
    for (int i = 0; i < 5; i++) {
        int bx = lx + i * 28;
        Color bCol = (stats.foodDropMult == fBtns[i]) ? Color{80, 255, 120, 255} : Color{50, 50, 70, 255};
        DrawRectangle(bx, ly, 24, 14, bCol);
        std::snprintf(buf, sizeof(buf), "%dx", fBtns[i]);
        DrawText(buf, bx + 2, ly + 2, 8, (stats.foodDropMult == fBtns[i]) ? Color{0, 0, 0, 255} : Color{200, 200, 200, 255});
    }
    ly += 20;

    // ── Keybindings ─────────────────────────────────────────────
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 6;
    DrawText("[SPC] pause  [</> ] rew/ff", lx, ly, 7, Color{60, 60, 90, 255});
    ly += 10;
    DrawText("[S]ave [L]oad [G]report",   lx, ly, 7, Color{60, 60, 90, 255});
    ly += 10;
    DrawText("[F11] fullscreen  [ESC]",   lx, ly, 7, Color{60, 60, 90, 255});
    ly += 14;

    // ── Status message (on simulation canvas, not panel) ────────
    if (statusMsg && statusMsg[0] != '\0') {
        int msgW = MeasureText(statusMsg, 16);
        int msgX = panelX / 2 - msgW / 2;
        DrawRectangle(msgX - 12, 8, msgW + 24, 28, Color{10, 10, 18, 220});
        DrawRectangleLines(msgX - 12, 8, msgW + 24, 28, Color{80, 120, 200, 180});
        DrawText(statusMsg, msgX, 14, 16, Color{100, 200, 255, 255});
    }
}
