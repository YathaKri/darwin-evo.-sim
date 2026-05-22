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

        // ── Click-to-select organism ────────────────────────────────
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            // Only detect clicks in the simulation canvas (not the UI panel)
            if (mouse.x < (float)panelX) {
                selectedCreatureId = -1;
                selectedCreature = nullptr;
                float closestDist = 9999.f;

                for (const auto& c : sim.getCreatures()) {
                    float dx = mouse.x - c->position.x;
                    float dy = mouse.y - c->position.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    // Click within the creature's body or a generous click area
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
            sim.update();
            ui.update(sim.getStats());
        } 
        else if (mode == PlayMode::FastForward) {
            // Run twice per frame
            sim.update();
            sim.update();
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
