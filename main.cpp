#include "raylib.h"
#include "Simulation.h"
#include "UI.h"
#include <string>
#include <iostream>

// ── Input mode for disaster placement ───────────────────────────
enum class InputMode { Normal, PlacingMeteor, PlacingNuke };

int main() {
    // Enable window resizing
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1040, 640, "Darwin Evolution Simulator - 5C^2 Hackathon");
    SetTargetFPS(60);

    Simulation sim;
    sim.init(80, 0.8f, 150);
    UI ui;

    PlayMode mode = PlayMode::Playing;
    
    // Click-to-select state
    int selectedCreatureId = -1;
    const Creature* selectedCreature = nullptr;
    
    // Status message overlay
    std::string statusMsg = "";
    int statusTimer = 0;

    auto showStatus = [&](const std::string& msg) {
        statusMsg = msg;
        statusTimer = 180; // 3 seconds at 60fps
    };

    Camera2D camera = { 0 };
    camera.target = (Vector2){ 0.0f, 0.0f };
    camera.offset = (Vector2){ 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // ── Disaster placement state ────────────────────────────────
    InputMode inputMode = InputMode::Normal;
    std::vector<Vector2> meteorTargets;

    // ── Camera tracking state ───────────────────────────────────
    bool isTrackingCreature = false;
    const float TRACK_ZOOM_TARGET = 2.5f;

    bool isExtinct = false;
    float extinctionRuntime = 0.f;

    while (!WindowShouldClose()) {
        // ── Extinction Check ────────────────────────────────────────
        if (!isExtinct && sim.getStats().population == 0) {
            isExtinct = true;
            extinctionRuntime = (float)GetTime();
            mode = PlayMode::Paused;
        }

        if (isExtinct) {
            mode = PlayMode::Paused;
        }

        // ── Window Resizing & Fullscreen ────────────────────────────
        
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // Pause simulation if the window is actively being resized
        if (IsWindowResized()) {
            mode = PlayMode::Paused;
            showStatus("Simulation Paused (Window Resized)");
        }

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        
        // Calculate new world bounds (leaving room for UI panel)
        int panelX = screenW - UI::PANEL_W;
        if (panelX < 400) panelX = 400; // minimum world width
        sim.setWorldSize((float)panelX, (float)screenH);

        // ── Input Handling ──────────────────────────────────────────
        
        // Play / Pause
        if (IsKeyPressed(KEY_SPACE)) {
            if (mode == PlayMode::Paused) mode = PlayMode::Playing;
            else mode = PlayMode::Paused;
        }

        // Rewind / Fast Forward
        if (IsKeyDown(KEY_LEFT)) {
            mode = PlayMode::Rewinding;
        } else if (IsKeyDown(KEY_RIGHT)) {
            mode = PlayMode::FastForward;
        } else if (mode == PlayMode::Rewinding || mode == PlayMode::FastForward) {
            mode = PlayMode::Playing;
        }

        // File I/O triggers
        if (IsKeyPressed(KEY_S)) {
            sim.saveGenomes("genomes_saved.txt");
            showStatus("Saved genomes to genomes_saved.txt");
        }
        if (IsKeyPressed(KEY_L)) {
            sim.loadGenomes("genomes_saved.txt");
            showStatus("Loaded genomes from genomes_saved.txt");
        }
        if (IsKeyPressed(KEY_G)) {
            sim.generateReport("simulation_report.txt");
            showStatus("Generated simulation_report.txt");
        }

        // ── Camera Panning & Zooming ────────────────────────────────
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            delta.x = delta.x * -1.0f / camera.zoom;
            delta.y = delta.y * -1.0f / camera.zoom;
            camera.target.x += delta.x;
            camera.target.y += delta.y;
            
            // Interrupt tracking if user pans manually
            if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                isTrackingCreature = false;
            }
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            camera.offset = GetMousePosition();
            camera.target = mouseWorldPos;
            const float zoomIncrement = 0.125f;
            camera.zoom += (wheel * zoomIncrement);
            if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            if (camera.zoom > 4.0f) camera.zoom = 4.0f;
            
            // Interrupt tracking if user zooms manually
            isTrackingCreature = false;
        }

        // ── Right-click / ESC: cancel placement or deselect ─────────
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (inputMode != InputMode::Normal) {
                inputMode = InputMode::Normal;
                meteorTargets.clear();
                showStatus("Disaster placement cancelled");
            } else {
                selectedCreatureId = -1;
                selectedCreature = nullptr;
                isTrackingCreature = false;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (inputMode != InputMode::Normal) {
                inputMode = InputMode::Normal;
                meteorTargets.clear();
                showStatus("Disaster placement cancelled");
            }
        }

        // ── Click-to-select organism or UI ──────────────────────────
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            
            // Check UI panel clicks
            if (mouse.x >= (float)panelX) {
                int lx = panelX + 12;

                // ── Controls base Y (must match UI.cpp layout) ──────
                int controlsBaseY = screenH - 215;

                // Sim Speed buttons: offset +6 (divider) +12 (label) = +18
                int sY = controlsBaseY + 18;
                if (mouse.y >= sY && mouse.y <= sY + 14) {
                    int sBtns[] = {1, 5, 10, 15, 20};
                    for (int i = 0; i < 5; i++) {
                        int bx = lx + i * 28;
                        if (mouse.x >= bx && mouse.x <= bx + 24) {
                            sim.setSimSpeedMult(sBtns[i]);
                            showStatus("Simulation Speed updated");
                        }
                    }
                }
                
                // Food Drop Rate buttons: offset +18 +20 +12 = +50
                int fY = controlsBaseY + 50;
                if (mouse.y >= fY && mouse.y <= fY + 14) {
                    int fBtns[] = {0, 1, 2, 5, 10, 20};
                    for (int i = 0; i < 6; i++) {
                        int bx = lx + i * 25;
                        if (mouse.x >= bx && mouse.x <= bx + 22) {
                            sim.setFoodDropMult(fBtns[i]);
                            showStatus(fBtns[i] == 0 ? "Food drops DISABLED!" : "Food Drop Rate updated");
                        }
                    }
                }

                // Disaster buttons: offset +50 +20 +6 (divider) +6 +12 (label) = +94
                int dY = controlsBaseY + 94;
                if (mouse.y >= dY && mouse.y <= dY + 28) {
                    bool canSpawn = sim.canSpawnDisaster() && inputMode == InputMode::Normal;
                    if (canSpawn) {
                        for (int i = 0; i < 3; i++) {
                            int bx = lx + i * 50;
                            if (mouse.x >= bx && mouse.x <= bx + 44) {
                                if (i == 0) {
                                    // METEOR: enter placement mode
                                    inputMode = InputMode::PlacingMeteor;
                                    meteorTargets.clear();
                                    showStatus("METEOR: Click 3 locations on the map");
                                } else if (i == 1) {
                                    // VOLCANO: spawn immediately at random position
                                    sim.spawnDisaster(Disaster::createVolcano(
                                        sim.getRng(), sim.getWorldW(), sim.getWorldH()));
                                    showStatus("Volcano spawned! Eruption in 5 seconds...");
                                } else if (i == 2) {
                                    // NUKE: enter placement mode
                                    inputMode = InputMode::PlacingNuke;
                                    showStatus("NUKE: Click to mark ground zero");
                                }
                                break;
                            }
                        }
                    } else if (inputMode != InputMode::Normal) {
                        showStatus("Finish current placement first!");
                    } else {
                        showStatus("Disaster on cooldown!");
                    }
                }

            } else {
                // ── Click in simulation canvas ──────────────────────
                Vector2 worldMouse = GetScreenToWorld2D(mouse, camera);

                if (inputMode == InputMode::PlacingMeteor) {
                    // Place meteor target
                    meteorTargets.push_back(worldMouse);
                    if ((int)meteorTargets.size() >= 3) {
                        sim.spawnDisaster(Disaster::createMeteor(meteorTargets));
                        meteorTargets.clear();
                        inputMode = InputMode::Normal;
                        showStatus("Meteor strike incoming!");
                    } else {
                        char msgBuf[64];
                        std::snprintf(msgBuf, sizeof(msgBuf), "METEOR: Click %d more location(s)",
                                      3 - (int)meteorTargets.size());
                        showStatus(msgBuf);
                    }

                } else if (inputMode == InputMode::PlacingNuke) {
                    // Place nuke target
                    sim.spawnDisaster(Disaster::createNuke(worldMouse));
                    inputMode = InputMode::Normal;
                    showStatus("Nuclear strike incoming! 5 seconds to detonation!");

                } else {
                    // Normal click-to-select creature
                    selectedCreatureId = -1;
                    selectedCreature = nullptr;
                    float closestDist = 9999.f;

                    for (const auto& c : sim.getCreatures()) {
                        float dx = worldMouse.x - c->position.x;
                        float dy = worldMouse.y - c->position.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        float clickRadius = c->radius + 6.f;
                        if (dist < clickRadius && dist < closestDist) {
                            closestDist = dist;
                            selectedCreatureId = c->id;
                        }
                    }
                    
                    if (selectedCreatureId != -1) {
                        isTrackingCreature = true;
                    } else {
                        isTrackingCreature = false;
                    }
                }
            }
        }

        
        // ── Camera Tracking Update ──────────────────────────────────
        if (isTrackingCreature && selectedCreature) {
            // Smoothly move target to creature's position
            float lerpSpeed = 0.1f;
            camera.target.x += (selectedCreature->position.x - camera.target.x) * lerpSpeed;
            camera.target.y += (selectedCreature->position.y - camera.target.y) * lerpSpeed;
            
            // Set offset to center of screen (excluding UI panel)
            camera.offset.x = (float)panelX / 2.0f;
            camera.offset.y = (float)screenH / 2.0f;
            
            // Smoothly zoom in
            camera.zoom += (TRACK_ZOOM_TARGET - camera.zoom) * lerpSpeed;
        }

        // Status timer
        if (statusTimer > 0) {
            statusTimer--;
        } else {
            statusMsg = "";
        }

        // ── Simulation Logic Update ─────────────────────────────────
        
        if (mode == PlayMode::Playing) {
            int speed = sim.getSimSpeedMult();
            for (int i = 0; i < speed; ++i) {
                sim.update();
            }
            ui.update(sim.getStats());
        } 
        else if (mode == PlayMode::FastForward) {
            int speed = sim.getSimSpeedMult() * 2;
            for (int i = 0; i < speed; ++i) {
                sim.update();
            }
            ui.update(sim.getStats());
        } 
        else if (mode == PlayMode::Rewinding) {
            // Restores history state
            sim.rewindOneStep();
            ui.popLastHistory();
        }

        // Refresh the pointer each frame AFTER update (creature may have died)
        if (selectedCreatureId != -1) {
            selectedCreature = sim.findCreature(selectedCreatureId);
            if (!selectedCreature) {
                selectedCreatureId = -1;  // died, deselect
                isTrackingCreature = false;
            }
        }

        // ── Render ──────────────────────────────────────────────────
        
        BeginDrawing();
        ClearBackground(Color{15, 15, 20, 255}); // Dark slate background

        BeginMode2D(camera);

        // Draw grid
        int gw = (int)sim.getWorldW();
        int gh = (int)sim.getWorldH();
        for (int i = 0; i < gw; i += 50) DrawLine(i, 0, i, gh, Color{25, 25, 30, 255});
        for (int j = 0; j < gh; j += 50) DrawLine(0, j, gw, j, Color{25, 25, 30, 255});
        
        // Draw world bounds
        DrawRectangleLines(0, 0, gw, gh, Color{60, 60, 80, 255});

        // Draw entities (Simulation handles polymorphism Creature vs Hazard)
        sim.draw();

        // Highlight selected creature
        if (selectedCreature) {
            Vector2 pos = selectedCreature->getPosition();
            float r = selectedCreature->getRadius();
            // Pulsing selection ring
            float pulse = 0.7f + 0.3f * std::sin((float)GetTime() * 5.f);
            unsigned char alpha = (unsigned char)(180 * pulse);
            DrawCircleLines((int)pos.x, (int)pos.y, r + 6.f, {0, 255, 120, alpha});
            DrawCircleLines((int)pos.x, (int)pos.y, r + 7.f, {0, 255, 120, (unsigned char)(alpha / 2)});

            // "SELECTED" label
            const char* label = "SELECTED";
            int textW = MeasureText(label, 10);
            DrawText(label, (int)pos.x - textW / 2, (int)pos.y - (int)r - 16, 10, {0, 255, 120, 200});
        }

        // ── Draw disaster placement previews ────────────────────────
        if (inputMode == InputMode::PlacingMeteor) {
            Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), camera);

            // Preview circle at cursor
            float curPulse = 0.5f + 0.5f * std::sin((float)GetTime() * 4.f);
            DrawCircleLines((int)worldMouse.x, (int)worldMouse.y, 100.f,
                            {255, 100, 30, (unsigned char)(80 + 60 * curPulse)});
            DrawCircleV(worldMouse, 4.f, {255, 130, 40, 200});

            // Draw already-placed targets
            for (const auto& t : meteorTargets) {
                DrawCircleV(t, 5.f, {255, 100, 30, 220});
                DrawCircleLines((int)t.x, (int)t.y, 100.f, {255, 80, 20, 80});
                DrawCircleLines((int)t.x, (int)t.y, 6.f, {255, 150, 50, 160});
            }

        } else if (inputMode == InputMode::PlacingNuke) {
            Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), camera);
            float curPulse = 0.5f + 0.5f * std::sin((float)GetTime() * 3.f);

            // Outer damage ring
            DrawCircleLines((int)worldMouse.x, (int)worldMouse.y, 250.f,
                            {255, 200, 0, (unsigned char)(50 + 40 * curPulse)});
            // Inner kill zone
            DrawCircleLines((int)worldMouse.x, (int)worldMouse.y, 200.f,
                            {255, 50, 20, (unsigned char)(60 + 50 * curPulse)});
            // Center crosshair
            DrawLineEx({worldMouse.x - 10, worldMouse.y}, {worldMouse.x + 10, worldMouse.y},
                       1.5f, {255, 200, 0, 180});
            DrawLineEx({worldMouse.x, worldMouse.y - 10}, {worldMouse.x, worldMouse.y + 10},
                       1.5f, {255, 200, 0, 180});
            DrawCircleV(worldMouse, 3.f, {255, 220, 50, 220});
        }

        EndMode2D();

        // Build disaster UI info
        DisasterUIInfo dInfo;
        dInfo.cooldownFrames     = sim.getDisasterCooldown();
        dInfo.maxCooldown        = Disaster::COOLDOWN_FRAMES;
        dInfo.inputMode          = (inputMode == InputMode::PlacingMeteor) ? 1 :
                                   (inputMode == InputMode::PlacingNuke)   ? 2 : 0;
        dInfo.meteorTargetsPlaced = (int)meteorTargets.size();

        SimStats currentStats = sim.getStats();

        // Draw UI overlay panel (dynamically positioned based on window size)
        ui.draw(currentStats, mode, sim.historySize(),
                selectedCreature,
                statusMsg.empty() ? nullptr : statusMsg.c_str(),
                panelX, screenH,
                dInfo);

        // ── Simulation Ended Screen ─────────────────────────────────
        if (currentStats.population == 0) {
            DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 200});
            const char* title = "SIMULATION ENDED - EXTINCTION";
            int tw = MeasureText(title, 40);
            DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 100, 40, Color{255, 80, 80, 255});
            
            char lineBuf[128];
            int statY = screenH / 2 - 20;
            
            std::snprintf(lineBuf, sizeof(lineBuf), "Total Births: %d", currentStats.totalBirths);
            DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
            statY += 30;
            
            std::snprintf(lineBuf, sizeof(lineBuf), "Peak Population: %d", currentStats.peakPop);
            DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
            statY += 30;
            
            std::snprintf(lineBuf, sizeof(lineBuf), "Final Generation: %d", currentStats.generation);
            DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
            statY += 30;
            
            std::snprintf(lineBuf, sizeof(lineBuf), "Runtime: %.1f seconds", extinctionRuntime);
            DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
