#include "raylib.h"
#include "Simulation.h"
#include "UI.h"
#include <string>
#include <iostream>

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

    while (!WindowShouldClose()) {
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
        }

        // ── Click-to-select organism or UI ──────────────────────────
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            
            // Check UI panel clicks
            if (mouse.x >= (float)panelX) {
                int lx = panelX + 12;
                // These Y coordinates must match where they are drawn in UI.cpp
                // Sim Speed buttons Y ~ 280-320 (it's dynamic but let's approximate based on UI.cpp structure)
                // Actually, since UI is drawn dynamically, we need robust hitboxes.
                // Let's reverse-engineer the Y from UI.cpp:
                // Base ly starts at 10.
                // Title (16+8) = 34.
                // Playback (14+9+4+8+8) = 77.
                // Core stats (6 blocks * 28) = 245.
                // Avg stats (6+ 3 blocks * 24) = 323.
                // Graph (6+10+50+54) = 443.
                // Selected (varies heavily based on organism stats).
                // Wait, controls are drawn AFTER the selected organism info in UI.cpp.
                // This means their Y coordinate changes depending on the selected organism.
                // Instead of guessing, we can iterate to see if mouse clicked on the right side panel and check Y manually.
                // But wait, the controls are now ABOVE keybindings. Let's just find the approximate Y from bottom.
                // Keybindings height = 6+10+10+14 = 40.
                // Controls height = 6 + 12+20 (speed) + 12+20 (food) = 70.
                // So controls start at around screenH - 110.
                // Let's use screenH - 110 as a base for click detection.
                int controlsY = screenH - 130; 
                
                // Sim Speed (5 buttons)
                int sY = controlsY + 18;
                if (mouse.y >= sY && mouse.y <= sY + 14) {
                    int btns[] = {1, 5, 10, 15, 20};
                    for (int i = 0; i < 5; i++) {
                        int bx = lx + i * 28;
                        if (mouse.x >= bx && mouse.x <= bx + 24) {
                            sim.setSimSpeedMult(btns[i]);
                            showStatus("Simulation Speed updated");
                        }
                    }
                }
                
                // Food Drop Rate (5 buttons)
                int fY = controlsY + 50;
                if (mouse.y >= fY && mouse.y <= fY + 14) {
                    int fBtns[] = {1, 2, 5, 10, 20};
                    for (int i = 0; i < 5; i++) {
                        int bx = lx + i * 28;
                        if (mouse.x >= bx && mouse.x <= bx + 24) {
                            sim.setFoodDropMult(fBtns[i]);
                            showStatus("Food Drop Rate updated");
                        }
                    }
                }
            } 
            else {
                // Click in simulation canvas
                Vector2 worldMouse = GetScreenToWorld2D(mouse, camera);
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
            }
        }

        // Right-click or ESC to deselect
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
            selectedCreatureId = -1;
            selectedCreature = nullptr;
        }

        // Refresh the pointer each frame (creature may have died)
        if (selectedCreatureId != -1) {
            selectedCreature = sim.findCreature(selectedCreatureId);
            if (!selectedCreature) selectedCreatureId = -1;  // died, deselect
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

        EndMode2D();

        // Draw UI overlay panel (dynamically positioned based on window size)
        ui.draw(sim.getStats(), mode, sim.historySize(),
                selectedCreature,
                statusMsg.empty() ? nullptr : statusMsg.c_str(),
                panelX, screenH);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
