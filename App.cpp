#include "App.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

App::App()
    : m_state(AppState::STARTUP_MENU), m_previousState(AppState::STARTUP_MENU),
      m_isBorderlessFullscreen(true),
      m_playMode(PlayMode::Playing),
      m_selectedCreatureId(-1), m_selectedCreature(nullptr),
      m_statusMsg(""), m_statusTimer(0),
      m_isTrackingCreature(false), m_inputMode(InputMode::Normal),
      m_activeBgIndex(-1), m_bgmVolume(0.5f),
      m_isExtinct(false), m_extinctionRuntime(0.0f)
{
    m_currentMenuBgm.stream.buffer = nullptr;
    m_currentSimBgm.stream.buffer = nullptr;
    m_currentBg.id = 0;

    m_camera = { 0 };
    m_camera.target = (Vector2){ 0.0f, 0.0f };
    m_camera.offset = (Vector2){ 0.0f, 0.0f };
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

    InitAudioDevice();

    loadStats();
    loadResources();

    if (m_isBorderlessFullscreen) {
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        int display = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));
        SetWindowPosition(0, 0);
    }
}

App::~App() {
    saveStats();
    if (m_currentMenuBgm.stream.buffer != nullptr) UnloadMusicStream(m_currentMenuBgm);
    if (m_currentSimBgm.stream.buffer != nullptr) UnloadMusicStream(m_currentSimBgm);
    if (m_currentBg.id > 0) UnloadTexture(m_currentBg);
    CloseAudioDevice();
}

void App::loadStats() {
    std::ifstream file("stats.json");
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("\"lifetimeBirths\"") != std::string::npos) {
            sscanf(line.c_str(), "  \"lifetimeBirths\": %lld,", &m_appStats.lifetimeBirths);
        } else if (line.find("\"lifetimeDeaths\"") != std::string::npos) {
            sscanf(line.c_str(), "  \"lifetimeDeaths\": %lld,", &m_appStats.lifetimeDeaths);
        } else if (line.find("\"lifetimeDisasters\"") != std::string::npos) {
            sscanf(line.c_str(), "  \"lifetimeDisasters\": %lld,", &m_appStats.lifetimeDisasters);
        } else if (line.find("\"highestPopulation\"") != std::string::npos) {
            sscanf(line.c_str(), "  \"highestPopulation\": %d,", &m_appStats.highestPopulation);
        } else if (line.find("\"highestSpeed\"") != std::string::npos) {
            sscanf(line.c_str(), "  \"highestSpeed\": %f,", &m_appStats.highestSpeed);
        } else if (line.find("\"highestVision\"") != std::string::npos) {
            sscanf(line.c_str(), "  \"highestVision\": %f", &m_appStats.highestVision);
        }
    }
}

void App::saveStats() {
    std::ofstream file("stats.json");
    if (!file.is_open()) return;
    file << "{\n";
    file << "  \"lifetimeBirths\": " << m_appStats.lifetimeBirths << ",\n";
    file << "  \"lifetimeDeaths\": " << m_appStats.lifetimeDeaths << ",\n";
    file << "  \"lifetimeDisasters\": " << m_appStats.lifetimeDisasters << ",\n";
    file << "  \"highestPopulation\": " << m_appStats.highestPopulation << ",\n";
    file << "  \"highestSpeed\": " << m_appStats.highestSpeed << ",\n";
    file << "  \"highestVision\": " << m_appStats.highestVision << "\n";
    file << "}\n";
}

void App::loadResources() {
    auto scanDir = [](const std::string& path, std::vector<std::string>& files) {
        if (!fs::exists(path)) return;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    };

    scanDir("assets/bgm/menu", m_menuBgmFiles);
    scanDir("assets/bgm/sim", m_simBgmFiles);
    scanDir("assets/backgrounds", m_bgFiles);

    if (!m_bgFiles.empty()) {
        m_activeBgIndex = 0;
        m_currentBg = LoadTexture(m_bgFiles[m_activeBgIndex].c_str());
    }

    playRandomMenuBGM();
    playRandomSimBGM();
}

void App::playRandomMenuBGM() {
    if (m_menuBgmFiles.empty()) return;
    if (m_currentMenuBgm.stream.buffer != nullptr) UnloadMusicStream(m_currentMenuBgm);
    int idx = GetRandomValue(0, (int)m_menuBgmFiles.size() - 1);
    m_currentMenuBgm = LoadMusicStream(m_menuBgmFiles[idx].c_str());
    m_currentMenuBgm.looping = false;
    PlayMusicStream(m_currentMenuBgm);
    SetMusicVolume(m_currentMenuBgm, m_bgmVolume);
}

void App::playRandomSimBGM() {
    if (m_simBgmFiles.empty()) return;
    if (m_currentSimBgm.stream.buffer != nullptr) UnloadMusicStream(m_currentSimBgm);
    int idx = GetRandomValue(0, (int)m_simBgmFiles.size() - 1);
    m_currentSimBgm = LoadMusicStream(m_simBgmFiles[idx].c_str());
    m_currentSimBgm.looping = false;
    PlayMusicStream(m_currentSimBgm);
    SetMusicVolume(m_currentSimBgm, m_bgmVolume);
}

void App::updateMusicStream() {
    if (m_state == AppState::STARTUP_MENU || m_state == AppState::OPTIONS_MENU || m_state == AppState::STATS_MENU) {
        if (m_currentMenuBgm.stream.buffer != nullptr) {
            UpdateMusicStream(m_currentMenuBgm);
            if (GetMusicTimePlayed(m_currentMenuBgm) >= GetMusicTimeLength(m_currentMenuBgm)) {
                playRandomMenuBGM();
            }
        }
    } else {
        if (m_currentSimBgm.stream.buffer != nullptr) {
            UpdateMusicStream(m_currentSimBgm);
            if (GetMusicTimePlayed(m_currentSimBgm) >= GetMusicTimeLength(m_currentSimBgm)) {
                playRandomSimBGM();
            }
        }
    }
}

void App::showStatus(const std::string& msg) {
    m_statusMsg = msg;
    m_statusTimer = 180;
}

bool App::DrawButton(const char* text, int x, int y, int width, int height) {
    Rectangle rect = { (float)x, (float)y, (float)width, (float)height };
    Vector2 mouse = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mouse, rect);
    bool isClicked = isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    // Minecraft Java UI style colors
    Color baseColor = isHovered ? Color{100, 100, 100, 255} : Color{50, 50, 50, 255};
    Color borderColor = isHovered ? Color{255, 255, 255, 255} : Color{150, 150, 150, 255};
    Color textColor = isHovered ? Color{255, 255, 150, 255} : Color{200, 200, 200, 255};

    DrawRectangleRec(rect, baseColor);
    DrawRectangleLinesEx(rect, 2.0f, borderColor);

    int textW = MeasureText(text, 20);
    DrawText(text, x + width / 2 - textW / 2, y + height / 2 - 10, 20, textColor);

    return isClicked;
}

void App::run() {
    while (!WindowShouldClose()) {
        updateMusicStream();

        switch (m_state) {
            case AppState::STARTUP_MENU: updateStartupMenu(); break;
            case AppState::SIM_RUNNING:  updateSimRunning(); break;
            case AppState::SIM_PAUSED:   updateSimPaused(); break;
            case AppState::OPTIONS_MENU: updateOptionsMenu(); break;
            case AppState::STATS_MENU:   updateStatsMenu(); break;
        }

        BeginDrawing();
        ClearBackground(Color{15, 15, 20, 255});
        
        if (m_currentBg.id > 0) {
            // Draw scaled background to fit window
            DrawTexturePro(m_currentBg,
                Rectangle{0, 0, (float)m_currentBg.width, (float)m_currentBg.height},
                Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                Vector2{0, 0}, 0.0f, WHITE);
        }

        switch (m_state) {
            case AppState::STARTUP_MENU: drawStartupMenu(); break;
            case AppState::SIM_RUNNING:  drawSimRunning(); break;
            case AppState::SIM_PAUSED:   
                drawSimRunning(); // draw simulation in background
                drawSimPaused(); 
                break;
            case AppState::OPTIONS_MENU: drawOptionsMenu(); break;
            case AppState::STATS_MENU:   drawStatsMenu(); break;
        }

        EndDrawing();
    }
}

// ==============================================================================
// UPDATE METHODS
// ==============================================================================

void App::updateStartupMenu() {
    // Only UI updates needed, handled in draw
}

void App::updateSimRunning() {
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
        m_state = AppState::SIM_PAUSED;
        return;
    }

    if (!m_isExtinct && m_sim.getStats().population == 0) {
        m_isExtinct = true;
        m_extinctionRuntime = (float)GetTime();
        m_state = AppState::SIM_PAUSED;
    }

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int panelX = screenW - UI::PANEL_W;
    if (panelX < 400) panelX = 400;
    m_sim.setWorldSize((float)panelX, (float)screenH);

    // Camera Panning & Zooming
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta.x = delta.x * -1.0f / m_camera.zoom;
        delta.y = delta.y * -1.0f / m_camera.zoom;
        m_camera.target.x += delta.x;
        m_camera.target.y += delta.y;
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) m_isTrackingCreature = false;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), m_camera);
        m_camera.offset = GetMousePosition();
        m_camera.target = mouseWorldPos;
        const float zoomIncrement = 0.125f;
        m_camera.zoom += (wheel * zoomIncrement);
        if (m_camera.zoom < 0.1f) m_camera.zoom = 0.1f;
        if (m_camera.zoom > 4.0f) m_camera.zoom = 4.0f;
        m_isTrackingCreature = false;
    }

    // Input logic
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (m_inputMode != InputMode::Normal) {
            m_inputMode = InputMode::Normal;
            m_meteorTargets.clear();
            showStatus("Disaster placement cancelled");
        } else {
            m_selectedCreatureId = -1;
            m_selectedCreature = nullptr;
            m_isTrackingCreature = false;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (mouse.x >= (float)panelX) {
            int lx = panelX + 12;
            int controlsBaseY = screenH - 215;

            // Sim Speed buttons
            int sY = controlsBaseY + 18;
            if (mouse.y >= sY && mouse.y <= sY + 14) {
                int sBtns[] = {1, 5, 10, 15, 20};
                for (int i = 0; i < 5; i++) {
                    int bx = lx + i * 28;
                    if (mouse.x >= bx && mouse.x <= bx + 24) {
                        m_sim.setSimSpeedMult(sBtns[i]);
                        showStatus("Simulation Speed updated");
                    }
                }
            }
            
            // Food Drop Rate buttons
            int fY = controlsBaseY + 50;
            if (mouse.y >= fY && mouse.y <= fY + 14) {
                int fBtns[] = {0, 1, 2, 5, 10, 20};
                for (int i = 0; i < 6; i++) {
                    int bx = lx + i * 25;
                    if (mouse.x >= bx && mouse.x <= bx + 22) {
                        m_sim.setFoodDropMult(fBtns[i]);
                        showStatus(fBtns[i] == 0 ? "Food drops DISABLED!" : "Food Drop Rate updated");
                    }
                }
            }

            // Disaster buttons
            int dY = controlsBaseY + 94;
            if (mouse.y >= dY && mouse.y <= dY + 28) {
                bool canSpawn = m_sim.canSpawnDisaster() && m_inputMode == InputMode::Normal;
                if (canSpawn) {
                    for (int i = 0; i < 3; i++) {
                        int bx = lx + i * 50;
                        if (mouse.x >= bx && mouse.x <= bx + 44) {
                            if (i == 0) {
                                m_inputMode = InputMode::PlacingMeteor;
                                m_meteorTargets.clear();
                                showStatus("METEOR: Click 3 locations on the map");
                            } else if (i == 1) {
                                m_sim.spawnDisaster(Disaster::createVolcano(m_sim.getRng(), m_sim.getWorldW(), m_sim.getWorldH()));
                                showStatus("Volcano spawned! Eruption in 5 seconds...");
                                m_appStats.lifetimeDisasters++;
                            } else if (i == 2) {
                                m_inputMode = InputMode::PlacingNuke;
                                showStatus("NUKE: Click to mark ground zero");
                            }
                            break;
                        }
                    }
                } else if (m_inputMode != InputMode::Normal) {
                    showStatus("Finish current placement first!");
                } else {
                    showStatus("Disaster on cooldown!");
                }
            }
        } else {
            Vector2 worldMouse = GetScreenToWorld2D(mouse, m_camera);
            if (m_inputMode == InputMode::PlacingMeteor) {
                m_meteorTargets.push_back(worldMouse);
                if ((int)m_meteorTargets.size() >= 3) {
                    m_sim.spawnDisaster(Disaster::createMeteor(m_meteorTargets));
                    m_meteorTargets.clear();
                    m_inputMode = InputMode::Normal;
                    showStatus("Meteor strike incoming!");
                    m_appStats.lifetimeDisasters++;
                } else {
                    char msgBuf[64];
                    std::snprintf(msgBuf, sizeof(msgBuf), "METEOR: Click %d more location(s)", 3 - (int)m_meteorTargets.size());
                    showStatus(msgBuf);
                }
            } else if (m_inputMode == InputMode::PlacingNuke) {
                m_sim.spawnDisaster(Disaster::createNuke(worldMouse));
                m_inputMode = InputMode::Normal;
                showStatus("Nuclear strike incoming!");
                m_appStats.lifetimeDisasters++;
            } else {
                m_selectedCreatureId = -1;
                m_selectedCreature = nullptr;
                float closestDist = 9999.f;
                for (const auto& c : m_sim.getCreatures()) {
                    float dx = worldMouse.x - c->position.x;
                    float dy = worldMouse.y - c->position.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    float clickRadius = c->radius + 6.f;
                    if (dist < clickRadius && dist < closestDist) {
                        closestDist = dist;
                        m_selectedCreatureId = c->id;
                    }
                }
                m_isTrackingCreature = (m_selectedCreatureId != -1);
            }
        }
    }

    if (m_isTrackingCreature && m_selectedCreature) {
        float lerpSpeed = 0.1f;
        m_camera.target.x += (m_selectedCreature->position.x - m_camera.target.x) * lerpSpeed;
        m_camera.target.y += (m_selectedCreature->position.y - m_camera.target.y) * lerpSpeed;
        m_camera.offset.x = (float)panelX / 2.0f;
        m_camera.offset.y = (float)screenH / 2.0f;
        m_camera.zoom += (TRACK_ZOOM_TARGET - m_camera.zoom) * lerpSpeed;
    }

    if (m_statusTimer > 0) {
        m_statusTimer--;
    } else {
        m_statusMsg = "";
    }

    // Rewind / Fast Forward logic
    if (IsKeyDown(KEY_LEFT)) {
        m_playMode = PlayMode::Rewinding;
    } else if (IsKeyDown(KEY_RIGHT)) {
        m_playMode = PlayMode::FastForward;
    } else if (m_playMode == PlayMode::Rewinding || m_playMode == PlayMode::FastForward) {
        m_playMode = PlayMode::Playing;
    }

    // File I/O triggers
    if (IsKeyPressed(KEY_S)) {
        m_sim.saveGenomes("genomes_saved.txt");
        showStatus("Saved genomes to genomes_saved.txt");
    }
    if (IsKeyPressed(KEY_L)) {
        m_sim.loadGenomes("genomes_saved.txt");
        showStatus("Loaded genomes from genomes_saved.txt");
    }
    if (IsKeyPressed(KEY_G)) {
        m_sim.generateReport("simulation_report.txt");
        showStatus("Generated simulation_report.txt");
    }

    // Simulation Update
    int preBirths = m_sim.getStats().totalBirths;
    int popBefore = m_sim.getStats().population;

    if (m_playMode == PlayMode::Playing) {
        int speed = m_sim.getSimSpeedMult();
        for (int i = 0; i < speed; ++i) {
            m_sim.update();
        }
        m_ui.update(m_sim.getStats());
    } 
    else if (m_playMode == PlayMode::FastForward) {
        int speed = m_sim.getSimSpeedMult() * 2;
        for (int i = 0; i < speed; ++i) {
            m_sim.update();
        }
        m_ui.update(m_sim.getStats());
    } 
    else if (m_playMode == PlayMode::Rewinding) {
        m_sim.rewindOneStep();
        m_ui.popLastHistory();
    }

    // Update stats
    auto currentStats = m_sim.getStats();
    m_appStats.lifetimeBirths += (currentStats.totalBirths - preBirths);
    // Rough death tracking: population decreased and not newly born
    if (currentStats.population < popBefore) {
        m_appStats.lifetimeDeaths += (popBefore - currentStats.population) + (currentStats.totalBirths - preBirths);
    }
    if (currentStats.peakPop > m_appStats.highestPopulation) m_appStats.highestPopulation = currentStats.peakPop;
    if (currentStats.avgSpeed > m_appStats.highestSpeed) m_appStats.highestSpeed = currentStats.avgSpeed;
    if (currentStats.avgVision > m_appStats.highestVision) m_appStats.highestVision = currentStats.avgVision;

    if (m_selectedCreatureId != -1) {
        m_selectedCreature = m_sim.findCreature(m_selectedCreatureId);
        if (!m_selectedCreature) {
            m_selectedCreatureId = -1;
            m_isTrackingCreature = false;
        }
    }
}

void App::updateSimPaused() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
        if (!m_isExtinct) {
            m_state = AppState::SIM_RUNNING;
        }
    }
}

void App::updateOptionsMenu() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        m_state = m_previousState;
    }
}

void App::updateStatsMenu() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        m_state = m_previousState;
    }
}

// ==============================================================================
// DRAWING METHODS
// ==============================================================================

void App::drawStartupMenu() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    const char* title = "DARWIN EVOLUTION SIMULATOR";
    int tw = MeasureText(title, 40);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 200, 40, WHITE);

    int btnW = 300;
    int btnH = 40;
    int startY = screenH / 2 - 50;

    if (DrawButton("Start Simulation", screenW / 2 - btnW / 2, startY, btnW, btnH)) {
        m_sim.init(80, 0.8f, 150);
        m_isExtinct = false;
        m_appStats.lifetimeBirths += 80;
        m_state = AppState::SIM_RUNNING;
        if (m_currentSimBgm.stream.buffer != nullptr) PlayMusicStream(m_currentSimBgm);
    }
    if (DrawButton("Options", screenW / 2 - btnW / 2, startY + 60, btnW, btnH)) {
        m_previousState = AppState::STARTUP_MENU;
        m_state = AppState::OPTIONS_MENU;
    }
    if (DrawButton("Stats", screenW / 2 - btnW / 2, startY + 120, btnW, btnH)) {
        m_previousState = AppState::STARTUP_MENU;
        m_state = AppState::STATS_MENU;
    }
    if (DrawButton("Exit Game", screenW / 2 - btnW / 2, startY + 180, btnW, btnH)) {
        saveStats();
        CloseWindow();
        exit(0);
    }
}

void App::drawSimRunning() {
    if (m_currentBg.id <= 0) {
        ClearBackground(Color{15, 15, 20, 255});
    }

    BeginMode2D(m_camera);

    int gw = (int)m_sim.getWorldW();
    int gh = (int)m_sim.getWorldH();
    for (int i = 0; i < gw; i += 50) DrawLine(i, 0, i, gh, Color{25, 25, 30, 255});
    for (int j = 0; j < gh; j += 50) DrawLine(0, j, gw, j, Color{25, 25, 30, 255});
    
    DrawRectangleLines(0, 0, gw, gh, Color{60, 60, 80, 255});

    m_sim.draw();

    if (m_selectedCreature) {
        Vector2 pos = m_selectedCreature->getPosition();
        float r = m_selectedCreature->getRadius();
        float pulse = 0.7f + 0.3f * std::sin((float)GetTime() * 5.f);
        unsigned char alpha = (unsigned char)(180 * pulse);
        DrawCircleLines((int)pos.x, (int)pos.y, r + 6.f, {0, 255, 120, alpha});
        DrawCircleLines((int)pos.x, (int)pos.y, r + 7.f, {0, 255, 120, (unsigned char)(alpha / 2)});
        const char* label = "SELECTED";
        int textW = MeasureText(label, 10);
        DrawText(label, (int)pos.x - textW / 2, (int)pos.y - (int)r - 16, 10, {0, 255, 120, 200});
    }

    if (m_inputMode == InputMode::PlacingMeteor) {
        Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), m_camera);
        float curPulse = 0.5f + 0.5f * std::sin((float)GetTime() * 4.f);
        DrawCircleLines((int)worldMouse.x, (int)worldMouse.y, 100.f, {255, 100, 30, (unsigned char)(80 + 60 * curPulse)});
        DrawCircleV(worldMouse, 4.f, {255, 130, 40, 200});
        for (const auto& t : m_meteorTargets) {
            DrawCircleV(t, 5.f, {255, 100, 30, 220});
            DrawCircleLines((int)t.x, (int)t.y, 100.f, {255, 80, 20, 80});
            DrawCircleLines((int)t.x, (int)t.y, 6.f, {255, 150, 50, 160});
        }
    } else if (m_inputMode == InputMode::PlacingNuke) {
        Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), m_camera);
        float curPulse = 0.5f + 0.5f * std::sin((float)GetTime() * 3.f);
        DrawCircleLines((int)worldMouse.x, (int)worldMouse.y, 250.f, {255, 200, 0, (unsigned char)(50 + 40 * curPulse)});
        DrawCircleLines((int)worldMouse.x, (int)worldMouse.y, 200.f, {255, 50, 20, (unsigned char)(60 + 50 * curPulse)});
        DrawLineEx({worldMouse.x - 10, worldMouse.y}, {worldMouse.x + 10, worldMouse.y}, 1.5f, {255, 200, 0, 180});
        DrawLineEx({worldMouse.x, worldMouse.y - 10}, {worldMouse.x, worldMouse.y + 10}, 1.5f, {255, 200, 0, 180});
        DrawCircleV(worldMouse, 3.f, {255, 220, 50, 220});
    }

    EndMode2D();

    DisasterUIInfo dInfo;
    dInfo.cooldownFrames = m_sim.getDisasterCooldown();
    dInfo.maxCooldown = Disaster::COOLDOWN_FRAMES;
    dInfo.inputMode = (m_inputMode == InputMode::PlacingMeteor) ? 1 : (m_inputMode == InputMode::PlacingNuke) ? 2 : 0;
    dInfo.meteorTargetsPlaced = (int)m_meteorTargets.size();

    auto currentStats = m_sim.getStats();
    m_ui.draw(currentStats, m_playMode, m_sim.historySize(), m_selectedCreature,
              m_statusMsg.empty() ? nullptr : m_statusMsg.c_str(),
              GetScreenWidth() - UI::PANEL_W, GetScreenHeight(), dInfo);


}

void App::drawSimPaused() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 200});

    if (m_isExtinct) {
        const char* title = "SIMULATION ENDED - EXTINCTION";
        int tw = MeasureText(title, 40);
        DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 200, 40, Color{255, 80, 80, 255});
        
        auto currentStats = m_sim.getStats();
        char lineBuf[128];
        int statY = screenH / 2 - 130;
        
        std::snprintf(lineBuf, sizeof(lineBuf), "Total Births: %d", currentStats.totalBirths);
        DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        statY += 30;
        std::snprintf(lineBuf, sizeof(lineBuf), "Peak Population: %d", currentStats.peakPop);
        DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        statY += 30;
        std::snprintf(lineBuf, sizeof(lineBuf), "Final Generation: %d", currentStats.generation);
        DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        statY += 30;
        std::snprintf(lineBuf, sizeof(lineBuf), "Runtime: %.1f seconds", m_extinctionRuntime);
        DrawText(lineBuf, screenW / 2 - MeasureText(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        
        int btnW = 300;
        int btnH = 40;
        int startYBtn = screenH / 2 + 50;
        
        if (DrawButton("New Simulation", screenW / 2 - btnW / 2, startYBtn, btnW, btnH)) {
            m_sim.init(80, 0.8f, 150);
            m_isExtinct = false;
            m_appStats.lifetimeBirths += 80;
            m_state = AppState::SIM_RUNNING;
        }
        if (DrawButton("Main Menu", screenW / 2 - btnW / 2, startYBtn + 60, btnW, btnH)) {
            m_state = AppState::STARTUP_MENU;
            if (m_currentMenuBgm.stream.buffer != nullptr) PlayMusicStream(m_currentMenuBgm);
        }
    } else {
        const char* title = "GAME PAUSED";
        int tw = MeasureText(title, 40);
        DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 150, 40, WHITE);

        int btnW = 300;
        int btnH = 40;
        int startYBtn = screenH / 2 - 50;

        if (DrawButton("Continue", screenW / 2 - btnW / 2, startYBtn, btnW, btnH)) {
            m_state = AppState::SIM_RUNNING;
        }
        
        if (DrawButton("Options", screenW / 2 - btnW / 2, startYBtn + 60, btnW, btnH)) {
            m_previousState = AppState::SIM_PAUSED;
            m_state = AppState::OPTIONS_MENU;
        }

        if (DrawButton("Main Menu", screenW / 2 - btnW / 2, startYBtn + 120, btnW, btnH)) {
            m_state = AppState::STARTUP_MENU;
            if (m_currentMenuBgm.stream.buffer != nullptr) PlayMusicStream(m_currentMenuBgm);
        }
    }
}

void App::drawOptionsMenu() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 200});

    const char* title = "OPTIONS";
    int tw = MeasureText(title, 40);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 200, 40, WHITE);

    int btnW = 400;
    int btnH = 40;
    int startY = screenH / 2 - 100;

    // BGM Volume Slider (custom basic implementation)
    DrawText("BGM Volume:", screenW / 2 - btnW / 2, startY, 20, WHITE);
    Rectangle sliderBar = { (float)(screenW / 2 - btnW / 2 + 150), (float)startY, (float)(btnW - 150), 20.0f };
    DrawRectangleRec(sliderBar, Color{50, 50, 50, 255});
    
    float filledW = sliderBar.width * m_bgmVolume;
    DrawRectangleRec(Rectangle{sliderBar.x, sliderBar.y, filledW, sliderBar.height}, Color{100, 200, 100, 255});
    
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, Rectangle{sliderBar.x, sliderBar.y - 10, sliderBar.width, sliderBar.height + 20})) {
            m_bgmVolume = (mouse.x - sliderBar.x) / sliderBar.width;
            if (m_bgmVolume < 0.0f) m_bgmVolume = 0.0f;
            if (m_bgmVolume > 1.0f) m_bgmVolume = 1.0f;
            SetMusicVolume(m_currentMenuBgm, m_bgmVolume);
            SetMusicVolume(m_currentSimBgm, m_bgmVolume);
        }
    }
    
    char volStr[16];
    std::snprintf(volStr, sizeof(volStr), "%d%%", (int)(m_bgmVolume * 100));
    DrawText(volStr, (int)sliderBar.x + (int)sliderBar.width + 10, startY, 20, WHITE);

    // Cycle Background
    if (DrawButton("Cycle Background", screenW / 2 - btnW / 2, startY + 60, btnW, btnH)) {
        if (!m_bgFiles.empty()) {
            m_activeBgIndex = (m_activeBgIndex + 1) % m_bgFiles.size();
            if (m_currentBg.id > 0) UnloadTexture(m_currentBg);
            m_currentBg = LoadTexture(m_bgFiles[m_activeBgIndex].c_str());
        }
    }

    // Window Mode
    const char* winModeTxt = m_isBorderlessFullscreen ? "Window Mode: Borderless" : "Window Mode: Windowed";
    if (DrawButton(winModeTxt, screenW / 2 - btnW / 2, startY + 120, btnW, btnH)) {
        m_isBorderlessFullscreen = !m_isBorderlessFullscreen;
        if (m_isBorderlessFullscreen) {
            SetWindowState(FLAG_WINDOW_UNDECORATED);
            int display = GetCurrentMonitor();
            SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));
            SetWindowPosition(0, 0);
        } else {
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowSize(1040, 640);
            SetWindowPosition(100, 100);
        }
    }

    if (DrawButton("Done", screenW / 2 - 100, startY + 220, 200, btnH)) {
        m_state = m_previousState;
    }
}

void App::drawStatsMenu() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 200});

    const char* title = "STATISTICS";
    int tw = MeasureText(title, 40);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 200, 40, WHITE);

    int startY = screenH / 2 - 100;
    int col1X = screenW / 2 - 250;
    int col2X = screenW / 2 + 50;
    
    DrawText("Lifetime Cumulative Totals", col1X, startY, 20, Color{150, 255, 150, 255});
    DrawText(TextFormat("Births: %lld", m_appStats.lifetimeBirths), col1X, startY + 40, 20, WHITE);
    DrawText(TextFormat("Deaths: %lld", m_appStats.lifetimeDeaths), col1X, startY + 70, 20, WHITE);
    DrawText(TextFormat("Disasters: %lld", m_appStats.lifetimeDisasters), col1X, startY + 100, 20, WHITE);

    DrawText("All-Time High Peaks", col2X, startY, 20, Color{150, 150, 255, 255});
    DrawText(TextFormat("Population: %d", m_appStats.highestPopulation), col2X, startY + 40, 20, WHITE);
    DrawText(TextFormat("Avg Speed: %.2f", m_appStats.highestSpeed), col2X, startY + 70, 20, WHITE);
    DrawText(TextFormat("Avg Vision: %.2f", m_appStats.highestVision), col2X, startY + 100, 20, WHITE);

    if (DrawButton("Done", screenW / 2 - 100, startY + 200, 200, 40)) {
        m_state = m_previousState;
    }
}
