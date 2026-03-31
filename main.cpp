#include "raylib.h"
#include "Simulation.h"
#include "UI.h"

int main() {
    const int screenW = Simulation::WORLD_W + UI::PANEL_W;
    const int screenH = Simulation::WORLD_H;

    SetConfigFlags(FLAG_MSAA_4X_HINT);   // anti-aliasing for smooth circles
    InitWindow(screenW, screenH, "Evolution Simulator");
    SetTargetFPS(60);

    Simulation sim;
    UI         ui;

    bool     paused = false;
    PlayMode mode   = PlayMode::Playing;

    static constexpr int FF_MULTIPLIER = 5;   // how many updates per frame when fast-forwarding

    while (!WindowShouldClose()) {
        // ── Input ──────────────────────────────────────────────
        if (IsKeyPressed(KEY_SPACE))
            paused = !paused;

        bool fastForward = !paused && IsKeyDown(KEY_RIGHT);
        bool rewinding   = IsKeyDown(KEY_LEFT);          // works even when paused

        // Determine display mode
        if (rewinding)              mode = PlayMode::Rewinding;
        else if (paused)            mode = PlayMode::Paused;
        else if (fastForward)       mode = PlayMode::FastForward;
        else                        mode = PlayMode::Playing;

        // ── Simulation tick ────────────────────────────────────
        if (rewinding) {
            // Rewind: restore previous snapshot and reverse the graph
            sim.rewindOneStep();
            ui.popLastHistory();
        } else if (paused) {
            // Don't update simulation — just keep drawing

        } else if (fastForward) {
            // Run multiple updates per frame
            for (int i = 0; i < FF_MULTIPLIER; i++) {
                sim.update();
                ui.update(sim.getStats());   // record each sub-step for the graph
            }

        } else {
            // Normal play
            sim.update();
            ui.update(sim.getStats());
        }

        // ── Draw ───────────────────────────────────────────────
        BeginDrawing();
            ClearBackground(Color{14, 14, 20, 255});

            // Draw a subtle grid over the sim area
            for (int x = 0; x < (int)Simulation::WORLD_W; x += 60)
                DrawLine(x, 0, x, screenH, Color{255, 255, 255, 6});
            for (int y = 0; y < screenH; y += 60)
                DrawLine(0, y, (int)Simulation::WORLD_W, y, Color{255, 255, 255, 6});

            sim.draw();
            ui.draw(sim.getStats(), mode, sim.historySize());

            // Overlay flash when rewinding (subtle VHS-style effect)
            if (rewinding) {
                DrawRectangle(0, 0, (int)Simulation::WORLD_W, screenH,
                              Color{255, 80, 100, 12});
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
