#include "UI.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>

Font g_mainFont;

void DrawTextCustom(const char *text, int posX, int posY, int fontSize, Color color) {
    DrawTextEx(g_mainFont, text, Vector2{(float)posX, (float)posY}, (float)fontSize, (float)fontSize/10.0f, color);
}

int MeasureTextCustom(const char *text, int fontSize) {
    return (int)MeasureTextEx(g_mainFont, text, (float)fontSize, (float)fontSize/10.0f).x;
}
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
    DrawTextCustom(label, x, y, 16, Color{90, 90, 120, 255});
    y += 18;
    DrawTextCustom(value, x, y, 26, Color{230, 230, 240, 255});
    y += 32;
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
              int screenW, int screenH,
              const DisasterUIInfo& disasterInfo) const {
    int px = screenW - RIGHT_PANEL_W;
    int pw = RIGHT_PANEL_W;
    int bh = BOTTOM_PANEL_H;
    int by = screenH - bh;

    // ── Right Panel background ────────────────────────────────────────
    DrawRectangle(px, 0, pw, screenH, Color{10, 10, 18, 245});
    DrawRectangle(px, 0, 1,  screenH, Color{45, 45, 65, 255});

    // ── Bottom Panel background ───────────────────────────────────────
    DrawRectangle(0, by, px, bh, Color{10, 10, 18, 245});
    DrawRectangle(0, by, px, 1, Color{45, 45, 65, 255});

    int lx = px + 12;
    int ly = 12;

    // ── Title ───────────────────────────────────────────────────
    DrawTextCustom("EVOLUTION", lx, ly, 16, Color{80, 120, 200, 255});
    ly += 22;
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 12;

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

        DrawTextCustom(label, lx, ly, 14, col);
        ly += 20;

        // Rewind buffer bar
        DrawTextCustom("REWIND BUFFER", lx, ly, 10, Color{70, 70, 100, 255});
        ly += 14;
        float bufRatio = std::clamp((float)historyFrames / 600.f, 0.f, 1.f);
        DrawRectangle(lx, ly, pw - 24, 6, Color{30, 30, 45, 255});
        Color barCol = (mode == PlayMode::Rewinding)
                           ? Color{255, 100, 130, 200}
                           : Color{80, 80, 120, 200};
        DrawRectangle(lx, ly, (int)((pw - 24) * bufRatio), 6, barCol);
        ly += 12;

        DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
        ly += 12;
    }

    // ── Core stats ──────────────────────────────────────────────
    char buf[64];

    std::snprintf(buf, sizeof(buf), "%d", stats.population);
    drawStatBlock("POPULATION", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.peakPop);
    drawStatBlock("PEAK", buf, lx + 120, ly -= 38); ly += 38;

    std::snprintf(buf, sizeof(buf), "%d", stats.foodCount);
    drawStatBlock("FOOD", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.totalBirths);
    drawStatBlock("BIRTHS", buf, lx + 120, ly -= 38); ly += 38;

    std::snprintf(buf, sizeof(buf), "%d", stats.totalFights);
    drawStatBlock("FIGHTS", buf, lx, ly);

    std::snprintf(buf, sizeof(buf), "%d", stats.hazardCount);
    drawStatBlock("HAZARDS", buf, lx + 120, ly -= 38); ly += 38;

    std::snprintf(buf, sizeof(buf), "%d", stats.generation);
    drawStatBlock("GEN", buf, lx, ly);

    // ── Avg traits with generation comparison ───────────────────
    auto drawAvgStat = [&](const char* name, float val, float prevVal,
                           bool hasPrev, float barMax, Color bCol) {
        DrawTextCustom(name, lx, ly, 12, Color{90, 90, 120, 255});
        ly += 14;

        std::snprintf(buf, sizeof(buf), "%.1f", val);
        DrawTextCustom(buf, lx, ly, 18, Color{230, 230, 240, 255});

        // Trend delta
        if (hasPrev) {
            float delta = val - prevVal;
            char deltaBuf[32];
            std::snprintf(deltaBuf, sizeof(deltaBuf), "%+.1f", delta);
            Color dc = (delta >= 0) ? Color{80, 255, 130, 200} : Color{255, 80, 80, 200};
            DrawTextCustom(deltaBuf, lx + 60, ly + 4, 12, dc);
        }
        ly += 22;

        drawBar(val, barMax, bCol, lx, ly, pw - 24);
        ly += 14;
    };

    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 8;

    drawAvgStat("AVG SIZE",   stats.avgSize,   stats.gen1AvgSize,
                stats.hasGen1, 14.f,  Color{120, 100, 255, 255});
    drawAvgStat("AVG SPEED",  stats.avgSpeed,  stats.gen1AvgSpeed,
                stats.hasGen1, 8.f,   Color{255, 100, 150, 255});
    drawAvgStat("AVG VISION", stats.avgVision, stats.gen1AvgVision,
                stats.hasGen1, 300.f, Color{255, 200, 80, 255});

    // ── Population graph ────────────────────────────────────────
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 8;
    DrawTextCustom("POP HISTORY", lx, ly, 12, Color{90, 90, 120, 255});
    ly += 16;
    drawGraph(lx, ly, pw - 24, 60);
    ly += 68;

    // ── Selected organism info (Right Panel) ────────────────────
    DrawLine(lx, ly, px + pw - 12, ly, Color{40, 40, 60, 255});
    ly += 8;

    if (selected) {
        DrawTextCustom("SELECTED", lx, ly, 20, Color{0, 255, 120, 255});
        ly += 24;

        // ID
        std::snprintf(buf, sizeof(buf), "ID: %d", selected->id);
        DrawTextCustom(buf, lx, ly, 18, Color{230, 230, 240, 255});
        ly += 22;

        // HP bar
        DrawTextCustom("HP", lx, ly, 14, Color{90, 90, 120, 255});
        ly += 18;
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", selected->hp, selected->maxHp);
        DrawTextCustom(buf, lx, ly, 18, Color{230, 230, 240, 255});
        ly += 22;
        float hpRatio = std::clamp(selected->hp / selected->maxHp, 0.f, 1.f);
        Color hpCol = (hpRatio > 0.5f)  ? Color{80, 255, 100, 220}
                     : (hpRatio > 0.25f) ? Color{255, 200, 60, 220}
                     :                     Color{255, 60, 60, 220};
        drawBar(selected->hp, selected->maxHp, hpCol, lx, ly, pw - 24);
        ly += 14;

        // Energy bar
        DrawTextCustom("ENERGY", lx, ly, 14, Color{90, 90, 120, 255});
        ly += 18;
        std::snprintf(buf, sizeof(buf), "%.0f", selected->energy);
        DrawTextCustom(buf, lx, ly, 18, Color{230, 230, 240, 255});
        ly += 22;
        drawBar(selected->energy, 200.f, Color{255, 200, 80, 220}, lx, ly, pw - 24);
        ly += 14;

        // Age
        std::snprintf(buf, sizeof(buf), "Age: %d", selected->age);
        DrawTextCustom(buf, lx, ly, 16, Color{180, 180, 200, 255});
        ly += 20;

        // Size / Speed / Vision
        float spd = std::sqrt(selected->velocity.x * selected->velocity.x +
                              selected->velocity.y * selected->velocity.y);
        std::snprintf(buf, sizeof(buf), "Size: %.1f", selected->radius);
        DrawTextCustom(buf, lx, ly, 16, Color{180, 180, 200, 255});
        ly += 20;
        std::snprintf(buf, sizeof(buf), "Speed: %.1f", spd);
        DrawTextCustom(buf, lx, ly, 16, Color{180, 180, 200, 255});
        ly += 20;
        std::snprintf(buf, sizeof(buf), "Vision: %.0f", selected->visionRange);
        DrawTextCustom(buf, lx, ly, 16, Color{180, 180, 200, 255});
        ly += 20;

        // Fitness
        std::snprintf(buf, sizeof(buf), "Fitness: %.1f", selected->genome.fitness());
        DrawTextCustom(buf, lx, ly, 16, Color{100, 200, 255, 255});
        ly += 20;

        // Shape
        const char* shapeNames[] = {"Circle", "Triangle", "Diamond", "Pentagon", "Hexagon"};
        int si = (int)selected->shape;
        if (si >= 0 && si <= 4)
            std::snprintf(buf, sizeof(buf), "Shape: %s", shapeNames[si]);
        else
            std::snprintf(buf, sizeof(buf), "Shape: ???");
        DrawTextCustom(buf, lx, ly, 16, Color{180, 180, 200, 255});
        ly += 20;

        // DOT status
        if (selected->dotTimer > 0) {
            std::snprintf(buf, sizeof(buf), "POISONED (%.0fs)", (float)selected->dotTimer / 60.f);
            DrawTextCustom(buf, lx, ly, 16, Color{160, 40, 255, 255});
            ly += 20;
        }

        // Mating status
        if (selected->mateTargetId != -1) {
            std::snprintf(buf, sizeof(buf), "Seeking mate #%d", selected->mateTargetId);
            DrawTextCustom(buf, lx, ly, 16, Color{255, 105, 180, 255});
            ly += 20;
        }

        // Flee status
        if (selected->fleeing) {
            DrawTextCustom("FLEEING HAZARD!", lx, ly, 16, Color{255, 80, 80, 255});
            ly += 20;
        }
    } else {
        DrawTextCustom("INSPECT", lx, ly, 18, Color{90, 90, 120, 255});
        ly += 24;
        DrawTextCustom("Click an organism", lx, ly, 16, Color{60, 60, 90, 255});
        ly += 20;
        DrawTextCustom("Right-click to deselect", lx, ly, 14, Color{50, 50, 75, 255});
        ly += 22;
    }

    // ═══════════════════════════════════════════════════════════
    //  BOTTOM SECTION (Mutations, Badges, Disasters, Speeds)
    // ═══════════════════════════════════════════════════════════
    
    // Sim Speed & Food Drop Rate
    int sX = 20;
    int sY = by + 12;
    
    DrawTextCustom("SIM SPEED", sX, sY, 16, Color{90, 90, 120, 255});
    sY += 24;
    int btns[] = {1, 5, 10, 15, 20};
    for (int i = 0; i < 5; i++) {
        int bx = sX + i * 50;
        Color bCol = (stats.simSpeedMult == btns[i]) ? Color{100, 200, 255, 255} : Color{50, 50, 70, 255};
        DrawRectangle(bx, sY, 44, 28, bCol);
        std::snprintf(buf, sizeof(buf), "%dx", btns[i]);
        int tw = MeasureTextCustom(buf, 14);
        DrawTextCustom(buf, bx + 22 - tw/2, sY + 7, 14, (stats.simSpeedMult == btns[i]) ? Color{0, 0, 0, 255} : Color{200, 200, 200, 255});
    }

    int fY = by + 78;
    DrawTextCustom("FOOD DROP RATE", sX, fY, 16, Color{90, 90, 120, 255});
    fY += 24;
    int fBtns[] = {0, 1, 2, 5, 10, 20};
    for (int i = 0; i < 6; i++) {
        int bx = sX + i * 50;
        Color bCol = (stats.foodDropMult == fBtns[i]) ? Color{80, 255, 120, 255} : Color{50, 50, 70, 255};
        DrawRectangle(bx, fY, 44, 28, bCol);
        std::snprintf(buf, sizeof(buf), "%dx", fBtns[i]);
        int tw = MeasureTextCustom(buf, 14);
        DrawTextCustom(buf, bx + 22 - tw/2, fY + 7, 14, (stats.foodDropMult == fBtns[i]) ? Color{0, 0, 0, 255} : Color{200, 200, 200, 255});
    }

    // ── DISASTERS SECTION ───────────────────────────────────────
    int dX = (px / 2) - 145; // Centered
    int dY = by + 12;

    int titleW = MeasureTextCustom("DISASTERS", 18);
    DrawTextCustom("DISASTERS", dX + 145 - titleW / 2, dY, 18, Color{255, 80, 50, 255});
    dY += 28;

    bool canSpawn = (disasterInfo.cooldownFrames <= 0 && disasterInfo.inputMode == 0);

    // 3 disaster buttons: 60x40 each, 10px gap
    struct DisBtn { const char* label; Color activeColor; };
    DisBtn dBtns[3] = {
        {"METEOR",   {200, 100, 30, 255}},
        {"VOLCANO",  {200, 50,  20, 255}},
        {"NUKE",     {200, 180, 0,  255}},
    };

    for (int i = 0; i < 3; i++) {
        int bx = dX + i * 100;

        Color bgCol  = canSpawn ? dBtns[i].activeColor : Color{35, 35, 50, 255};
        Color txtCol = canSpawn ? Color{255, 255, 255, 255} : Color{80, 80, 100, 255};

        if ((i == 0 && disasterInfo.inputMode == 1) ||
            (i == 2 && disasterInfo.inputMode == 2)) {
            bgCol = {255, 255, 100, 255};
            txtCol = {0, 0, 0, 255};
        }

        DrawRectangle(bx, dY, 90, 60, bgCol);
        DrawRectangleLines(bx, dY, 90, 60, Color{(unsigned char)(bgCol.r/2 + 60),
                                                   (unsigned char)(bgCol.g/2 + 60),
                                                   (unsigned char)(bgCol.b/2 + 60), 200});

        // Draw icons (scaled up slightly)
        float cx = bx + 45.f;
        float cy = dY + 24.f;

        if (i == 0) {
            DrawCircleV({cx, cy}, 15, {30, 80, 180, canSpawn ? (unsigned char)220 : (unsigned char)80});
            DrawCircleV({cx - 4.5f, cy - 4.5f}, 7.5f, {40, 160, 60, canSpawn ? (unsigned char)200 : (unsigned char)60});
            DrawCircleV({cx + 6, cy + 3}, 5.25f, {40, 140, 50, canSpawn ? (unsigned char)180 : (unsigned char)50});
            DrawCircleLines((int)cx, (int)cy, 15, {60, 120, 220, canSpawn ? (unsigned char)160 : (unsigned char)50});
        } else if (i == 1) {
            Vector2 vtop   = {cx, cy - 15};
            Vector2 vleft  = {cx - 21, cy + 15};
            Vector2 vright = {cx + 21, cy + 15};
            Color mtnCol = canSpawn ? Color{120, 65, 30, 255} : Color{50, 35, 20, 200};
            DrawTriangle(vtop, vleft, vright, mtnCol);
            DrawCircleV({cx, cy - 12}, 6, canSpawn ? Color{255, 80, 20, 220} : Color{100, 40, 20, 100});
            DrawLineV(vtop, vleft, canSpawn ? Color{160, 90, 45, 200} : Color{60, 40, 20, 100});
            DrawLineV(vleft, vright, canSpawn ? Color{160, 90, 45, 200} : Color{60, 40, 20, 100});
            DrawLineV(vright, vtop, canSpawn ? Color{160, 90, 45, 200} : Color{60, 40, 20, 100});
        } else {
            unsigned char a = canSpawn ? (unsigned char)220 : (unsigned char)70;
            DrawCircleV({cx, cy - 6}, 12, {255, 200, 0, a});
            DrawCircleV({cx - 7.5f, cy - 3}, 7.5f, {255, 180, 0, (unsigned char)(a * 0.8f)});
            DrawCircleV({cx + 7.5f, cy - 3}, 7.5f, {255, 180, 0, (unsigned char)(a * 0.8f)});
            DrawRectangle((int)(cx - 4.5f), (int)(cy + 1.5f), 9, 15, {220, 160, 0, (unsigned char)(a * 0.8f)});
            DrawCircleV({cx, cy + 15}, 9, {200, 140, 30, (unsigned char)(a * 0.5f)});
        }

        // Label below icon
        int tw = MeasureTextCustom(dBtns[i].label, 14);
        DrawTextCustom(dBtns[i].label, bx + 45 - tw / 2, dY + 45, 14, txtCol);
    }
    
    int cdY = dY + 48;
    // Cooldown bar
    if (disasterInfo.cooldownFrames > 0) {
        DrawTextCustom("COOLDOWN", dX + 45, cdY, 14, Color{90, 90, 120, 255});
        cdY += 18;
        float cdRatio = (float)disasterInfo.cooldownFrames / (float)disasterInfo.maxCooldown;
        DrawRectangle(dX + 45, cdY, 200, 6, Color{30, 30, 45, 255});
        DrawRectangle(dX + 45, cdY, (int)(200 * cdRatio), 6, Color{255, 80, 50, 200});
    } else if (disasterInfo.inputMode != 0) {
        const char* modeText = "";
        if (disasterInfo.inputMode == 1) {
            std::snprintf(buf, sizeof(buf), "METEOR: %d/3 placed", disasterInfo.meteorTargetsPlaced);
            modeText = buf;
        } else if (disasterInfo.inputMode == 2) {
            modeText = "NUKE: Click target";
        }
        int modeW = MeasureTextCustom(modeText, 16);
        DrawTextCustom(modeText, dX + 145 - modeW / 2, cdY, 16, Color{255, 255, 100, 255});
    }

    // ── SELECTED CREATURE MUTATIONS & BADGES (Bottom Right Section) ──
    int mX = px - 420;
    int mY = by + 12;

    if (selected) {
        // Badges
        if (selected->toxImmune || selected->radImmune || selected->famBadge) {
            DrawTextCustom("BADGES:", mX, mY, 18, Color{90, 90, 120, 255});
            mY += 24;
            if (selected->toxImmune) {
                float bx = mX + 6.f;
                float by_badge = mY + 6.f;
                float bs = 5.f;
                Vector2 top   = {bx,      by_badge - bs};
                Vector2 right = {bx + bs, by_badge};
                Vector2 bot   = {bx,      by_badge + bs};
                Vector2 left  = {bx - bs, by_badge};
                Color brightSilver = {240, 255, 255, 255};
                DrawTriangle(top, left, bot, brightSilver);
                DrawTriangle(top, bot, right, brightSilver);
                DrawTextCustom("  TOX Immune (Silver)", mX + 18, mY - 2, 16, brightSilver);
                mY += 22;
            }
            if (selected->radImmune) {
                float bx = mX + 6.f;
                float by_badge = mY + 6.f;
                float bs = 6.f;
                Color brightGold = {255, 255, 0, 255};
                for (int i = 0; i < 5; i++) {
                    float a1 = (2.f * 3.14159f / 5.f) * i - 3.14159f / 2.f;
                    float a2 = (2.f * 3.14159f / 5.f) * (i + 1) - 3.14159f / 2.f;
                    float aMid = (a1 + a2) / 2.f;
                    Vector2 outer1 = {bx + bs * std::cos(a1), by_badge + bs * std::sin(a1)};
                    Vector2 outer2 = {bx + bs * std::cos(a2), by_badge + bs * std::sin(a2)};
                    Vector2 inner  = {bx + bs * 0.4f * std::cos(aMid), by_badge + bs * 0.4f * std::sin(aMid)};
                    DrawTriangle({bx, by_badge}, outer1, inner, brightGold);
                    DrawTriangle({bx, by_badge}, inner, outer2, brightGold);
                }
                DrawTextCustom("  RAD Immune (Gold)", mX + 18, mY - 2, 16, brightGold);
                mY += 22;
            }
            if (selected->famBadge) {
                float bx = mX + 6.f;
                float by_badge = mY + 6.f;
                float bs = 5.5f;
                Color brightBronze = {255, 120, 30, 255};
                for (int i = 0; i < 6; i++) {
                    float a1 = (2.f * 3.14159f / 6.f) * i - 3.14159f / 2.f;
                    float a2 = (2.f * 3.14159f / 6.f) * (i + 1) - 3.14159f / 2.f;
                    Vector2 outer1 = {bx + bs * std::cos(a1), by_badge + bs * std::sin(a1)};
                    Vector2 outer2 = {bx + bs * std::cos(a2), by_badge + bs * std::sin(a2)};
                    DrawTriangle({bx, by_badge}, outer1, outer2, brightBronze);
                    DrawLineV(outer1, outer2, brightBronze);
                }
                DrawTextCustom("  FAM Survivor (Bronze)", mX + 18, mY - 2, 16, brightBronze);
                mY += 22;
            }
        }

        int mutX = mX + 220; // Shift mutations to the right of badges
        int mutY = by + 12;
        
        // Mutation stack display
        if (selected->mutations.hasMutations()) {
            DrawTextCustom("MUTATIONS", mutX, mutY, 18, Color{90, 90, 120, 255});
            mutY += 24;

            if (selected->mutations.speedCount > 0) {
                std::snprintf(buf, sizeof(buf), "Speed x%d (+%.0f%%)",
                         selected->mutations.speedCount, selected->mutations.speedBonus);
                DrawTextCustom(buf, mutX, mutY, 16, Color{100, 200, 255, 255});
                mutY += 22;
            }
            if (selected->mutations.visionCount > 0) {
                std::snprintf(buf, sizeof(buf), "Vision x%d (+%.0f%%)",
                         selected->mutations.visionCount, selected->mutations.visionBonus);
                DrawTextCustom(buf, mutX, mutY, 16, Color{255, 220, 100, 255});
                mutY += 22;
            }
            if (selected->mutations.staminaCount > 0) {
                std::snprintf(buf, sizeof(buf), "Stamina x%d (-%d%% cost)",
                         selected->mutations.staminaCount, selected->mutations.staminaCount * 5);
                DrawTextCustom(buf, mutX, mutY, 16, Color{255, 255, 80, 255});
                mutY += 22;
            }
            if (selected->mutations.sizeCount > 0) {
                std::snprintf(buf, sizeof(buf), "Size x%d (+%d%% body)",
                         selected->mutations.sizeCount, selected->mutations.sizeCount * 10);
                DrawTextCustom(buf, mutX, mutY, 16, Color{200, 120, 255, 255});
                mutY += 22;
            }
            if (selected->mutations.hasBio) {
                std::snprintf(buf, sizeof(buf), "Biolum. x%d", selected->mutations.bioCount);
                DrawTextCustom(buf, mutX, mutY, 16, selected->mutations.neonColor);
                mutY += 22;
            }
        }
    }

    // ── Keybindings ─────────────────────────────────────────────
    int kX = px - 250;
    int kY = by + 20;
    DrawTextCustom("[SPC] pause", kX, kY, 16, Color{60, 60, 90, 255});
    kY += 22;
    DrawTextCustom("[</>] rew/ff", kX, kY, 16, Color{60, 60, 90, 255});
    kY += 22;
    DrawTextCustom("[S]ave [L]oad", kX, kY, 16, Color{60, 60, 90, 255});
    kY += 22;
    DrawTextCustom("[G]report", kX, kY, 16, Color{60, 60, 90, 255});
    kY += 22;
    DrawTextCustom("[F11] fullscreen", kX, kY, 16, Color{60, 60, 90, 255});
    kY += 22;
    DrawTextCustom("[RMB] cancel", kX, kY, 16, Color{60, 60, 90, 255});

    // ── Status message (on simulation canvas, not panel) ────────
    if (statusMsg && statusMsg[0] != '\0') {
        int msgW = MeasureTextCustom(statusMsg, 16);
        int msgX = px / 2 - msgW / 2;
        DrawRectangle(msgX - 12, 8, msgW + 24, 28, Color{10, 10, 18, 220});
        DrawRectangleLines(msgX - 12, 8, msgW + 24, 28, Color{80, 120, 200, 180});
        DrawTextCustom(statusMsg, msgX, 14, 16, Color{100, 200, 255, 255});
    }
}
