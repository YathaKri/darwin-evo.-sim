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
              int panelX, int screenH,
              const DisasterUIInfo& disasterInfo) const {
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

    drawAvgStat("AVG SIZE",   stats.avgSize,   stats.gen1AvgSize,
                stats.hasGen1, 14.f,  Color{120, 100, 255, 255});
    drawAvgStat("AVG SPEED",  stats.avgSpeed,  stats.gen1AvgSpeed,
                stats.hasGen1, 8.f,   Color{255, 100, 150, 255});
    drawAvgStat("AVG VISION", stats.avgVision, stats.gen1AvgVision,
                stats.hasGen1, 300.f, Color{255, 200, 80, 255});

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

        // Immunity badges (vector icons)
        if (selected->toxImmune || selected->radImmune || selected->famBadge) {
            DrawText("BADGES:", lx, ly, 7, Color{90, 90, 120, 255});
            ly += 10;
            if (selected->toxImmune) {
                // Draw silver diamond badge
                float bx = lx + 4.f;
                float by = ly + 4.f;
                float bs = 3.f;
                Vector2 top   = {bx,      by - bs};
                Vector2 right = {bx + bs, by};
                Vector2 bot   = {bx,      by + bs};
                Vector2 left  = {bx - bs, by};
                DrawTriangle(top, left, bot, Color{192, 192, 210, 255});
                DrawTriangle(top, bot, right, Color{192, 192, 210, 255});
                DrawText("  TOX Immune (Silver)", lx + 8, ly, 9, Color{192, 192, 210, 255});
                ly += 12;
            }
            if (selected->radImmune) {
                // Draw gold star badge
                float bx = lx + 4.f;
                float by = ly + 4.f;
                float bs = 3.5f;
                Color gold = {255, 215, 0, 255};
                for (int i = 0; i < 5; i++) {
                    float a1 = (2.f * 3.14159f / 5.f) * i - 3.14159f / 2.f;
                    float a2 = (2.f * 3.14159f / 5.f) * (i + 1) - 3.14159f / 2.f;
                    float aMid = (a1 + a2) / 2.f;
                    Vector2 outer1 = {bx + bs * std::cos(a1), by + bs * std::sin(a1)};
                    Vector2 outer2 = {bx + bs * std::cos(a2), by + bs * std::sin(a2)};
                    Vector2 inner  = {bx + bs * 0.4f * std::cos(aMid), by + bs * 0.4f * std::sin(aMid)};
                    DrawTriangle({bx, by}, outer1, inner, gold);
                    DrawTriangle({bx, by}, inner, outer2, gold);
                }
                DrawText("  RAD Immune (Gold)", lx + 8, ly, 9, Color{255, 215, 0, 255});
                ly += 12;
            }
            if (selected->famBadge) {
                // Draw bronze hexagon badge (solid color)
                float bx = lx + 4.f;
                float by = ly + 4.f;
                float bs = 3.5f;
                Color bronze = {205, 127, 50, 255};
                for (int i = 0; i < 6; i++) {
                    float a1 = (2.f * 3.14159f / 6.f) * i - 3.14159f / 2.f;
                    float a2 = (2.f * 3.14159f / 6.f) * (i + 1) - 3.14159f / 2.f;
                    Vector2 outer1 = {bx + bs * std::cos(a1), by + bs * std::sin(a1)};
                    Vector2 outer2 = {bx + bs * std::cos(a2), by + bs * std::sin(a2)};
                    DrawTriangle({bx, by}, outer1, outer2, bronze);
                    DrawLineV(outer1, outer2, bronze);
                }
                DrawText("  FAM Survivor (Bronze)", lx + 8, ly, 9, Color{205, 127, 50, 255});
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

    // ═══════════════════════════════════════════════════════════
    //  BOTTOM SECTION (fixed position from bottom)
    // ═══════════════════════════════════════════════════════════

    ly = screenH - 215;

    // ── Controls ────────────────────────────────────────────────
    DrawRectangle(px + 1, ly - 4, pw - 1, screenH - ly + 4, Color{10, 10, 18, 245});
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
    int fBtns[] = {0, 1, 2, 5, 10, 20};
    for (int i = 0; i < 6; i++) {
        int bx = lx + i * 25;
        Color bCol = (stats.foodDropMult == fBtns[i]) ? Color{80, 255, 120, 255} : Color{50, 50, 70, 255};
        DrawRectangle(bx, ly, 22, 14, bCol);
        std::snprintf(buf, sizeof(buf), "%dx", fBtns[i]);
        DrawText(buf, bx + 2, ly + 2, 8, (stats.foodDropMult == fBtns[i]) ? Color{0, 0, 0, 255} : Color{200, 200, 200, 255});
    }
    ly += 20;

    // ── DISASTERS SECTION ───────────────────────────────────────
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 6;

    DrawText("DISASTERS", lx, ly, 8, Color{255, 80, 50, 255});
    ly += 12;

    bool canSpawn = (disasterInfo.cooldownFrames <= 0 && disasterInfo.inputMode == 0);

    // 3 disaster buttons: 44x28 each, 6px gap
    struct DisBtn { const char* label; Color activeColor; };
    DisBtn dBtns[3] = {
        {"MET",  {200, 100, 30, 255}},
        {"VOL",  {200, 50,  20, 255}},
        {"NUK",  {200, 180, 0,  255}},
    };

    for (int i = 0; i < 3; i++) {
        int bx = lx + i * 50;
        int by = ly;

        Color bgCol  = canSpawn ? dBtns[i].activeColor : Color{35, 35, 50, 255};
        Color txtCol = canSpawn ? Color{255, 255, 255, 255} : Color{80, 80, 100, 255};

        // Highlight if this disaster's placement mode is active
        if ((i == 0 && disasterInfo.inputMode == 1) ||
            (i == 2 && disasterInfo.inputMode == 2)) {
            bgCol = {255, 255, 100, 255};
            txtCol = {0, 0, 0, 255};
        }

        DrawRectangle(bx, by, 44, 28, bgCol);
        DrawRectangleLines(bx, by, 44, 28, Color{(unsigned char)(bgCol.r/2 + 60),
                                                   (unsigned char)(bgCol.g/2 + 60),
                                                   (unsigned char)(bgCol.b/2 + 60), 200});

        // Draw icons
        float cx = bx + 22.f;
        float cy = by + 11.f;

        if (i == 0) {
            // METEOR: Earth icon (blue/green sphere)
            DrawCircleV({cx, cy}, 8, {30, 80, 180, canSpawn ? (unsigned char)220 : (unsigned char)80});
            DrawCircleV({cx - 2, cy - 2}, 4, {40, 160, 60, canSpawn ? (unsigned char)200 : (unsigned char)60});
            DrawCircleV({cx + 3, cy + 1}, 2.5f, {40, 140, 50, canSpawn ? (unsigned char)180 : (unsigned char)50});
            DrawCircleLines((int)cx, (int)cy, 8, {60, 120, 220, canSpawn ? (unsigned char)160 : (unsigned char)50});
        } else if (i == 1) {
            // VOLCANO: Mountain triangle + red top
            Vector2 vtop   = {cx, cy - 8};
            Vector2 vleft  = {cx - 10, cy + 7};
            Vector2 vright = {cx + 10, cy + 7};
            Color mtnCol = canSpawn ? Color{120, 65, 30, 255} : Color{50, 35, 20, 200};
            DrawTriangle(vtop, vleft, vright, mtnCol);
            DrawCircleV({cx, cy - 6}, 3, canSpawn ? Color{255, 80, 20, 220} : Color{100, 40, 20, 100});
            DrawLineV(vtop, vleft, canSpawn ? Color{160, 90, 45, 200} : Color{60, 40, 20, 100});
            DrawLineV(vleft, vright, canSpawn ? Color{160, 90, 45, 200} : Color{60, 40, 20, 100});
            DrawLineV(vright, vtop, canSpawn ? Color{160, 90, 45, 200} : Color{60, 40, 20, 100});
        } else {
            // NUKE: Mushroom cloud
            unsigned char a = canSpawn ? (unsigned char)220 : (unsigned char)70;
            DrawCircleV({cx, cy - 3}, 6, {255, 200, 0, a});
            DrawCircleV({cx - 3, cy - 1}, 3.5f, {255, 180, 0, (unsigned char)(a * 0.8f)});
            DrawCircleV({cx + 3, cy - 1}, 3.5f, {255, 180, 0, (unsigned char)(a * 0.8f)});
            DrawRectangle((int)(cx - 2), (int)(cy + 1), 4, 7, {220, 160, 0, (unsigned char)(a * 0.8f)});
            DrawCircleV({cx, cy + 7}, 4, {200, 140, 30, (unsigned char)(a * 0.5f)});
        }

        // Label below icon
        int tw = MeasureText(dBtns[i].label, 7);
        DrawText(dBtns[i].label, bx + 22 - tw / 2, by + 21, 7, txtCol);
    }
    ly += 32;

    // Cooldown bar
    if (disasterInfo.cooldownFrames > 0) {
        DrawText("COOLDOWN", lx, ly, 7, Color{90, 90, 120, 255});
        ly += 9;
        float cdRatio = (float)disasterInfo.cooldownFrames / (float)disasterInfo.maxCooldown;
        DrawRectangle(lx, ly, pw - 24, 4, Color{30, 30, 45, 255});
        DrawRectangle(lx, ly, (int)((pw - 24) * cdRatio), 4, Color{255, 80, 50, 200});
        ly += 8;
    } else if (disasterInfo.inputMode != 0) {
        // Placement mode indicator
        const char* modeText = "";
        if (disasterInfo.inputMode == 1) {
            std::snprintf(buf, sizeof(buf), "METEOR: %d/3 placed", disasterInfo.meteorTargetsPlaced);
            modeText = buf;
        } else if (disasterInfo.inputMode == 2) {
            modeText = "NUKE: Click target";
        }
        DrawText(modeText, lx, ly, 7, Color{255, 255, 100, 255});
        ly += 10;
    } else {
        ly += 4;
    }

    // ── Keybindings ─────────────────────────────────────────────
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 6;
    DrawText("[SPC] pause  [</>] rew/ff", lx, ly, 7, Color{60, 60, 90, 255});
    ly += 10;
    DrawText("[S]ave [L]oad [G]report",   lx, ly, 7, Color{60, 60, 90, 255});
    ly += 10;
    DrawText("[F11] fullscreen  [ESC]",   lx, ly, 7, Color{60, 60, 90, 255});
    ly += 10;
    DrawText("[RMB] cancel placement",    lx, ly, 7, Color{60, 60, 90, 255});
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
