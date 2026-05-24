#include "raylib.h"
#include "App.h"

int main() {
    // Enable window resizing
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1040, 640, "Darwin Evolution Simulator - 5C^2 Hackathon");
    SetTargetFPS(60);

    App app;
    app.run();

    // Close window is handled inside App destructor or after run
    return 0;
}
