#include "raylib.h"
#include "Simulation.h"
#include "UI.h"

int main() {
    const int screenW = Simulation::WORLD_W + UI::PANEL_W;
    const int screenH = Simulation::WORLD_H;

    SetConfigFlags(FLAG_MSAA_4X_HINT);   // anti-aliasing for smooth circles
    InitWindow(screenW, screenH, "Evolution Simulator");
    SetTargetFPS(100);

    Simulation sim;
    UI         ui;

    while (!WindowShouldClose()) {
        sim.update();
        ui.update(sim.getStats());

        BeginDrawing();
            ClearBackground(Color{14, 14, 20, 255});

            // Draw a subtle grid over the sim area
            for (int x = 0; x < (int)Simulation::WORLD_W; x += 60)
                DrawLine(x, 0, x, screenH, Color{255, 255, 255, 6});
            for (int y = 0; y < screenH; y += 60)
                DrawLine(0, y, (int)Simulation::WORLD_W, y, Color{255, 255, 255, 6});

            sim.draw();
            ui.draw(sim.getStats());
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
