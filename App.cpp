#include "App.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

App::App()
    : m_state(AppState::SPLASH_SCREEN), m_previousState(AppState::STARTUP_MENU),
      m_isBorderlessFullscreen(true),
      m_playMode(PlayMode::Playing),
      m_selectedCreatureId(-1), m_selectedCreature(nullptr),
      m_statusMsg(""), m_statusTimer(0),
      m_isTrackingCreature(false), m_inputMode(InputMode::Normal),
      m_activeBgIndex(-1), m_bgmVolume(0.5f),
      m_isExtinct(false), m_extinctionRuntime(0.0f),
      m_splashTimer(0.0f), m_splashDuration(3.0f),
      m_loadingTimer(0.0f), m_loadingDuration(2.5f),
      m_loadingZoomPhase(0.0f), m_loadingDone(false),
      m_extinctAnimTimer(0.0f), m_extinctAnimDuration(3.5f),
      m_lastCreaturePos({0,0}), m_lastCreatureColor(WHITE), m_lastCreatureRadius(6.0f)
{
    m_currentMenuBgm.stream.buffer = nullptr;
    m_currentSimBgm.stream.buffer = nullptr;
    m_clickSound.stream.buffer = nullptr;
    m_currentBg.id = 0;
    m_currentBgImage.data = nullptr;
    m_isBgAnimated = false;
    m_bgAnimFrames = 0;
    m_bgCurrentFrame = 0;
    m_bgFrameDelay = 4;
    m_bgFrameCounter = 0;

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
    if (m_clickSound.stream.buffer != nullptr) UnloadSound(m_clickSound);
    if (m_currentBg.id > 0) UnloadTexture(m_currentBg);
    if (m_currentBgImage.data != nullptr) UnloadImage(m_currentBgImage);
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

    g_mainFont = LoadFontEx("assets/JetBrainsMono.ttf", 64, 0, 0);
    SetTextureFilter(g_mainFont.texture, TEXTURE_FILTER_BILINEAR);

    scanDir("assets/bgm/menu", m_menuBgmFiles);
    scanDir("assets/bgm/sim", m_simBgmFiles);
    scanDir("assets/backgrounds", m_bgFiles);
    scanDir("assets/sfx/clicks", m_clickSoundFiles);

    if (!m_bgFiles.empty()) {
        loadBackground(0);
    }
    if (!m_clickSoundFiles.empty()) {
        loadClickSound(0);
    }

    playRandomMenuBGM();
    playRandomSimBGM();
}

void App::loadBackground(int index) {
    if (index < 0 || index >= (int)m_bgFiles.size()) return;
    
    // Unload old
    if (m_currentBg.id > 0) {
        UnloadTexture(m_currentBg);
        m_currentBg.id = 0;
    }
    if (m_currentBgImage.data != nullptr) {
        UnloadImage(m_currentBgImage);
        m_currentBgImage.data = nullptr;
    }
    
    m_activeBgIndex = index;
    std::string path = m_bgFiles[index];
    
    // Check if gif
    if (path.length() >= 4 && path.substr(path.length() - 4) == ".gif") {
        m_isBgAnimated = true;
        m_bgAnimFrames = 0;
        m_bgCurrentFrame = 0;
        m_bgFrameCounter = 0;
        m_bgFrameDelay = 4; // update every 4 frames (15 FPS at 60Hz)
        
        m_currentBgImage = LoadImageAnim(path.c_str(), &m_bgAnimFrames);
        if (m_currentBgImage.data != nullptr) {
            m_currentBg = LoadTextureFromImage(m_currentBgImage);
        }
    } else {
        m_isBgAnimated = false;
        m_currentBg = LoadTexture(path.c_str());
    }
}

void App::loadClickSound(int index) {
    if (index < 0 || index >= (int)m_clickSoundFiles.size()) return;
    
    if (m_clickSound.stream.buffer != nullptr) {
        UnloadSound(m_clickSound);
        m_clickSound.stream.buffer = nullptr;
    }
    
    m_activeClickSoundIndex = index;
    m_clickSound = LoadSound(m_clickSoundFiles[index].c_str());
    SetSoundVolume(m_clickSound, 1.0f); // Default volume, could tie to a slider later
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
    if (m_state == AppState::SPLASH_SCREEN) return; // No music during splash
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

    int textW = MeasureTextCustom(text, 20);
    DrawTextCustom(text, x + width / 2 - textW / 2, y + height / 2 - 10, 20, textColor);

    if (isClicked && m_clickSound.stream.buffer != nullptr) {
        PlaySound(m_clickSound);
    }

    return isClicked;
}

void App::run() {
    while (!WindowShouldClose()) {
        updateMusicStream();

        switch (m_state) {
            case AppState::SPLASH_SCREEN:        updateSplashScreen(); break;
            case AppState::STARTUP_MENU:         updateStartupMenu(); break;
            case AppState::SIM_RUNNING:          updateSimRunning(); break;
            case AppState::SIM_PAUSED:           updateSimPaused(); break;
            case AppState::OPTIONS_MENU:         updateOptionsMenu(); break;
            case AppState::STATS_MENU:           updateStatsMenu(); break;
            case AppState::LOADING_SCREEN:       updateLoadingScreen(); break;
            case AppState::EXTINCTION_ANIMATION: updateExtinctionAnimation(); break;
        }

        if (m_isBgAnimated && m_currentBgImage.data != nullptr && m_currentBg.id > 0) {
            m_bgFrameCounter++;
            if (m_bgFrameCounter >= m_bgFrameDelay) {
                m_bgFrameCounter = 0;
                m_bgCurrentFrame++;
                if (m_bgCurrentFrame >= m_bgAnimFrames) m_bgCurrentFrame = 0;
                unsigned int nextFrameDataOffset = m_currentBgImage.width * m_currentBgImage.height * 4 * m_bgCurrentFrame;
                UpdateTexture(m_currentBg, ((unsigned char *)m_currentBgImage.data) + nextFrameDataOffset);
            }
        }

        BeginDrawing();
        ClearBackground(Color{15, 15, 20, 255});

        // Splash screen draws on pure black — no background texture
        if (m_state != AppState::SPLASH_SCREEN && m_currentBg.id > 0) {
            DrawTexturePro(m_currentBg,
                Rectangle{0, 0, (float)m_currentBg.width, (float)m_currentBg.height},
                Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                Vector2{0, 0}, 0.0f, WHITE);
        }

        switch (m_state) {
            case AppState::SPLASH_SCREEN:        drawSplashScreen(); break;
            case AppState::STARTUP_MENU:         drawStartupMenu(); break;
            case AppState::SIM_RUNNING:          drawSimRunning(); break;
            case AppState::SIM_PAUSED:           
                drawSimRunning(); // draw simulation in background
                drawSimPaused(); 
                break;
            case AppState::OPTIONS_MENU:         drawOptionsMenu(); break;
            case AppState::STATS_MENU:           drawStatsMenu(); break;
            case AppState::LOADING_SCREEN:       drawLoadingScreen(); break;
            case AppState::EXTINCTION_ANIMATION: 
                drawSimRunning(); // draw sim world behind the explosion
                drawExtinctionAnimation();
                break;
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

    if (!m_isExtinct) {
        // Track the last creature's data every frame while pop == 1
        auto& creatures = m_sim.getCreatures();
        if (creatures.size() == 1) {
            m_lastCreaturePos = creatures[0]->position;
            m_lastCreatureColor = creatures[0]->color;
            m_lastCreatureRadius = creatures[0]->radius;
        }
        if (m_sim.getStats().population == 0) {
            m_isExtinct = true;
            m_extinctionRuntime = (float)GetTime();
            m_extinctAnimTimer = 0.0f;
            m_state = AppState::EXTINCTION_ANIMATION;
            return;
        }
    }

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int rightPanelW = UI::RIGHT_PANEL_W;
    int bottomPanelH = UI::BOTTOM_PANEL_H;
    int simW = screenW - rightPanelW;
    if (simW < 400) simW = 400;
    int simH = screenH - bottomPanelH;
    if (simH < 300) simH = 300;
    
    m_sim.setWorldSize((float)simW, (float)simH);

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
        if (mouse.x >= (float)simW) {
            // Clicked in the Right Panel (No interactive buttons here right now)
        } else if (mouse.y >= (float)simH) {
            // Clicked in the Bottom Panel
            
            // Sim Speed buttons
            int sY = simH + 34; 
            int sX = 20;
            if (mouse.y >= sY && mouse.y <= sY + 20) {
                int sBtns[] = {1, 5, 10, 15, 20};
                for (int i = 0; i < 5; i++) {
                    int bx = sX + i * 38;
                    if (mouse.x >= bx && mouse.x <= bx + 32) {
                        m_sim.setSimSpeedMult(sBtns[i]);
                        showStatus("Simulation Speed updated");
                        if (m_clickSound.stream.buffer != nullptr) PlaySound(m_clickSound);
                    }
                }
            }
            
            // Food Drop Rate buttons
            int fY = simH + 94;
            int fX = 20;
            if (mouse.y >= fY && mouse.y <= fY + 20) {
                int fBtns[] = {0, 1, 2, 5, 10, 20};
                for (int i = 0; i < 6; i++) {
                    int bx = fX + i * 38;
                    if (mouse.x >= bx && mouse.x <= bx + 32) {
                        m_sim.setFoodDropMult(fBtns[i]);
                        showStatus(fBtns[i] == 0 ? "Food drops DISABLED!" : "Food Drop Rate updated");
                        if (m_clickSound.stream.buffer != nullptr) PlaySound(m_clickSound);
                    }
                }
            }
            
            // Disaster buttons
            int dY = simH + 40;
            int dX = 300;
            if (mouse.y >= dY && mouse.y <= dY + 40) {
                bool canSpawn = m_sim.canSpawnDisaster() && m_inputMode == InputMode::Normal;
                if (canSpawn) {
                    for (int i = 0; i < 3; i++) {
                        int bx = dX + i * 70;
                        if (mouse.x >= bx && mouse.x <= bx + 60) {
                            if (m_clickSound.stream.buffer != nullptr) PlaySound(m_clickSound);
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
            // Clicked in simulation world
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
        m_camera.offset.x = (float)simW / 2.0f;
        m_camera.offset.y = (float)simH / 2.0f;
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
    int tw = MeasureTextCustom(title, 70);
    DrawTextCustom(title, screenW / 2 - tw / 2, screenH / 2 - 250, 70, WHITE);

    int btnW = 300;
    int btnH = 40;
    int startY = screenH / 2 - 50;

    if (DrawButton("Start Simulation", screenW / 2 - btnW / 2, startY, btnW, btnH)) {
        m_sim.init(80, 0.8f, 150);
        m_isExtinct = false;
        m_appStats.lifetimeBirths += 80;
        m_loadingTimer = 0.0f;
        m_loadingDone = false;
        m_loadingZoomPhase = 0.0f;
        m_state = AppState::LOADING_SCREEN;
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
        int textW = MeasureTextCustom(label, 10);
        DrawTextCustom(label, (int)pos.x - textW / 2, (int)pos.y - (int)r - 16, 10, {0, 255, 120, 200});
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
              GetScreenWidth(), GetScreenHeight(), dInfo);


}

void App::drawSimPaused() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 200});

    if (m_isExtinct) {
        const char* title = "SIMULATION ENDED - EXTINCTION";
        int tw = MeasureTextCustom(title, 40);
        DrawTextCustom(title, screenW / 2 - tw / 2, screenH / 2 - 200, 40, Color{255, 80, 80, 255});
        
        auto currentStats = m_sim.getStats();
        char lineBuf[128];
        int statY = screenH / 2 - 130;
        
        std::snprintf(lineBuf, sizeof(lineBuf), "Total Births: %d", currentStats.totalBirths);
        DrawTextCustom(lineBuf, screenW / 2 - MeasureTextCustom(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        statY += 30;
        std::snprintf(lineBuf, sizeof(lineBuf), "Peak Population: %d", currentStats.peakPop);
        DrawTextCustom(lineBuf, screenW / 2 - MeasureTextCustom(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        statY += 30;
        std::snprintf(lineBuf, sizeof(lineBuf), "Final Generation: %d", currentStats.generation);
        DrawTextCustom(lineBuf, screenW / 2 - MeasureTextCustom(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        statY += 30;
        std::snprintf(lineBuf, sizeof(lineBuf), "Runtime: %.1f seconds", m_extinctionRuntime);
        DrawTextCustom(lineBuf, screenW / 2 - MeasureTextCustom(lineBuf, 20) / 2, statY, 20, Color{230, 230, 240, 255});
        
        int btnW = 300;
        int btnH = 40;
        int startYBtn = screenH / 2 + 50;
        
        if (DrawButton("New Simulation", screenW / 2 - btnW / 2, startYBtn, btnW, btnH)) {
            m_sim.init(80, 0.8f, 150);
            m_isExtinct = false;
            m_appStats.lifetimeBirths += 80;
            m_loadingTimer = 0.0f;
            m_loadingDone = false;
            m_loadingZoomPhase = 0.0f;
            m_state = AppState::LOADING_SCREEN;
        }
        if (DrawButton("Main Menu", screenW / 2 - btnW / 2, startYBtn + 60, btnW, btnH)) {
            m_state = AppState::STARTUP_MENU;
            if (m_currentMenuBgm.stream.buffer != nullptr) PlayMusicStream(m_currentMenuBgm);
        }
    } else {
        const char* title = "GAME PAUSED";
        int tw = MeasureTextCustom(title, 40);
        DrawTextCustom(title, screenW / 2 - tw / 2, screenH / 2 - 150, 40, WHITE);

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
    int tw = MeasureTextCustom(title, 40);
    DrawTextCustom(title, screenW / 2 - tw / 2, screenH / 2 - 200, 40, WHITE);

    int btnW = 400;
    int btnH = 40;
    int startY = screenH / 2 - 100;

    // BGM Volume Slider (custom basic implementation)
    DrawTextCustom("BGM Volume:", screenW / 2 - btnW / 2, startY, 20, WHITE);
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
    DrawTextCustom(volStr, (int)sliderBar.x + (int)sliderBar.width + 10, startY, 20, WHITE);

    // Cycle Background
    if (DrawButton("Cycle Background", screenW / 2 - btnW / 2, startY + 60, btnW, btnH)) {
        if (!m_bgFiles.empty()) {
            loadBackground((m_activeBgIndex + 1) % m_bgFiles.size());
        }
    }

    // Cycle Click Sound
    if (DrawButton("Cycle Click Sound", screenW / 2 - btnW / 2, startY + 120, btnW, btnH)) {
        if (!m_clickSoundFiles.empty()) {
            loadClickSound((m_activeClickSoundIndex + 1) % m_clickSoundFiles.size());
        }
    }

    // Skip Music
    if (DrawButton("Skip Menu Track", screenW / 2 - btnW - 10, startY + 180, btnW, btnH)) {
        playRandomMenuBGM();
    }
    if (DrawButton("Skip Sim Track", screenW / 2 + 10, startY + 180, btnW, btnH)) {
        playRandomSimBGM();
    }

    // Window Mode
    const char* winModeTxt = m_isBorderlessFullscreen ? "Window Mode: Borderless" : "Window Mode: Windowed";
    if (DrawButton(winModeTxt, screenW / 2 - btnW / 2, startY + 240, btnW, btnH)) {
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

    if (DrawButton("Done", screenW / 2 - 100, startY + 300, 200, btnH)) {
        m_state = m_previousState;
    }
}

void App::drawStatsMenu() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 200});

    const char* title = "STATISTICS";
    int tw = MeasureTextCustom(title, 40);
    DrawTextCustom(title, screenW / 2 - tw / 2, screenH / 2 - 200, 40, WHITE);

    int startY = screenH / 2 - 100;
    int col1X = screenW / 2 - 250;
    int col2X = screenW / 2 + 50;
    
    DrawTextCustom("Lifetime Cumulative Totals", col1X, startY, 20, Color{150, 255, 150, 255});
    DrawTextCustom(TextFormat("Births: %lld", m_appStats.lifetimeBirths), col1X, startY + 40, 20, WHITE);
    DrawTextCustom(TextFormat("Deaths: %lld", m_appStats.lifetimeDeaths), col1X, startY + 70, 20, WHITE);
    DrawTextCustom(TextFormat("Disasters: %lld", m_appStats.lifetimeDisasters), col1X, startY + 100, 20, WHITE);

    DrawTextCustom("All-Time High Peaks", col2X, startY, 20, Color{150, 150, 255, 255});
    DrawTextCustom(TextFormat("Population: %d", m_appStats.highestPopulation), col2X, startY + 40, 20, WHITE);
    DrawTextCustom(TextFormat("Avg Speed: %.2f", m_appStats.highestSpeed), col2X, startY + 70, 20, WHITE);
    DrawTextCustom(TextFormat("Avg Vision: %.2f", m_appStats.highestVision), col2X, startY + 100, 20, WHITE);

    if (DrawButton("Done", screenW / 2 - 100, startY + 200, 200, 40)) {
        m_state = m_previousState;
    }
}

// ==============================================================================
// SPLASH SCREEN ("5C^2" Studio Intro)
// ==============================================================================

void App::updateSplashScreen() {
    m_splashTimer += GetFrameTime();
    if (m_splashTimer >= m_splashDuration || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        m_state = AppState::STARTUP_MENU;
        playRandomMenuBGM();
    }
}

void App::drawSplashScreen() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    // Pure black background
    DrawRectangle(0, 0, screenW, screenH, BLACK);
    
    // Alpha: fade in over first 0.8s, hold for 1.4s, fade out over last 0.8s
    float alpha = 1.0f;
    float fadeInEnd = 0.8f;
    float fadeOutStart = m_splashDuration - 0.8f;
    
    if (m_splashTimer < fadeInEnd) {
        alpha = m_splashTimer / fadeInEnd;
    } else if (m_splashTimer > fadeOutStart) {
        alpha = 1.0f - (m_splashTimer - fadeOutStart) / 0.8f;
    }
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    
    unsigned char a = (unsigned char)(alpha * 255);
    
    // "5C^2" main title - large and centered
    const char* studio = "5C^2";
    int studioSize = 80;
    int studioW = MeasureTextCustom(studio, studioSize);
    DrawTextCustom(studio, screenW / 2 - studioW / 2, screenH / 2 - 50, studioSize, Color{255, 255, 255, a});
    
    // Subtle glow effect: draw slightly larger behind with reduced alpha
    unsigned char glowA = (unsigned char)(alpha * 80);
    DrawTextCustom(studio, screenW / 2 - studioW / 2 - 2, screenH / 2 - 52, studioSize, Color{100, 180, 255, glowA});
    DrawTextCustom(studio, screenW / 2 - studioW / 2 + 2, screenH / 2 - 48, studioSize, Color{100, 180, 255, glowA});
    
    // Subtitle
    const char* subtitle = "P R E S E N T S";
    int subSize = 20;
    int subW = MeasureTextCustom(subtitle, subSize);
    DrawTextCustom(subtitle, screenW / 2 - subW / 2, screenH / 2 + 50, subSize, Color{180, 180, 200, a});

    // Subtle particle sparkles
    float t = m_splashTimer;
    for (int i = 0; i < 12; i++) {
        float angle = (float)i * 0.523f + t * 0.5f;
        float dist = 120.f + 30.f * std::sin(t * 2.f + (float)i);
        float px = screenW / 2.f + std::cos(angle) * dist;
        float py = screenH / 2.f + std::sin(angle) * dist;
        float sparkSize = 2.f + 1.5f * std::sin(t * 3.f + (float)i * 1.3f);
        DrawCircleV({px, py}, sparkSize, Color{150, 200, 255, (unsigned char)(a * 0.6f)});
    }
}

// ==============================================================================
// LOADING SCREEN
// ==============================================================================

void App::updateLoadingScreen() {
    float dt = GetFrameTime();
    m_loadingTimer += dt;
    
    if (!m_loadingDone) {
        if (m_loadingTimer >= m_loadingDuration) {
            m_loadingDone = true;
            m_loadingZoomPhase = 0.0f;
            // Set up camera for zoom-out animation — start zoomed in at world center
            float worldW = m_sim.getWorldW();
            float worldH = m_sim.getWorldH();
            int simW = GetScreenWidth() - UI::RIGHT_PANEL_W;
            m_camera.target = {worldW / 2.f, worldH / 2.f};
            m_camera.offset = {(float)simW / 2.f, (float)(GetScreenHeight() - UI::BOTTOM_PANEL_H) / 2.f};
            m_camera.zoom = 4.0f; // start very zoomed in
        }
    } else {
        // Zoom-out animation phase
        m_loadingZoomPhase += dt;
        float zoomDuration = 1.2f;
        float progress = m_loadingZoomPhase / zoomDuration;
        if (progress > 1.0f) progress = 1.0f;
        
        // Smooth ease-out: fast at start, slow at end
        float eased = 1.0f - (1.0f - progress) * (1.0f - progress);
        m_camera.zoom = 4.0f - (4.0f - 1.0f) * eased; // zoom from 4.0 to 1.0
        
        if (m_loadingZoomPhase >= zoomDuration) {
            m_camera.zoom = 1.0f;
            m_state = AppState::SIM_RUNNING;
        }
    }
}

void App::drawLoadingScreen() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    if (m_loadingDone) {
        // During zoom-out, draw the actual sim world
        drawSimRunning();
        
        // Fading overlay during zoom
        float zoomDuration = 1.2f;
        float fadeAlpha = 1.0f - (m_loadingZoomPhase / zoomDuration);
        if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, (unsigned char)(fadeAlpha * 150)});
    } else {
        // Dark overlay
        DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 220});
        
        // Title
        const char* title = "GENERATING WORLD...";
        int tw = MeasureTextCustom(title, 30);
        float pulse = 0.7f + 0.3f * std::sin((float)GetTime() * 4.f);
        unsigned char titleAlpha = (unsigned char)(255 * pulse);
        DrawTextCustom(title, screenW / 2 - tw / 2, screenH / 2 - 60, 30, Color{200, 220, 255, titleAlpha});
        
        // Progress bar
        float progress = m_loadingTimer / m_loadingDuration;
        if (progress > 1.0f) progress = 1.0f;
        
        int barW = 400;
        int barH = 16;
        int barX = screenW / 2 - barW / 2;
        int barY = screenH / 2;
        
        // Bar background
        DrawRectangle(barX - 2, barY - 2, barW + 4, barH + 4, Color{80, 80, 100, 255});
        DrawRectangle(barX, barY, barW, barH, Color{30, 30, 40, 255});
        
        // Filled portion with gradient-like effect
        int fillW = (int)(barW * progress);
        DrawRectangle(barX, barY, fillW, barH, Color{60, 180, 120, 255});
        // Bright leading edge
        if (fillW > 2) {
            DrawRectangle(barX + fillW - 3, barY, 3, barH, Color{120, 255, 180, 255});
        }
        
        // Percentage text
        char pctBuf[16];
        std::snprintf(pctBuf, sizeof(pctBuf), "%d%%", (int)(progress * 100));
        int pctW = MeasureTextCustom(pctBuf, 18);
        DrawTextCustom(pctBuf, screenW / 2 - pctW / 2, barY + barH + 12, 18, Color{180, 180, 200, 255});
        
        // Flavor text
        const char* flavors[] = {
            "Spawning organisms...",
            "Scattering food sources...",
            "Calibrating mutations...",
            "Initializing hazard systems...",
            "Building ecosystem..."
        };
        int flavorIdx = (int)(progress * 4.99f);
        if (flavorIdx > 4) flavorIdx = 4;
        const char* flavor = flavors[flavorIdx];
        int flavorW = MeasureTextCustom(flavor, 16);
        DrawTextCustom(flavor, screenW / 2 - flavorW / 2, barY + barH + 40, 16, Color{120, 120, 150, 200});
    }
}

// ==============================================================================
// EXTINCTION ANIMATION (Dramatic Last Death)
// ==============================================================================

void App::updateExtinctionAnimation() {
    float dt = GetFrameTime();
    m_extinctAnimTimer += dt;
    
    // Lock camera onto the last creature's position and zoom in
    float lerpSpeed = 0.08f;
    int simW = GetScreenWidth() - UI::RIGHT_PANEL_W;
    m_camera.target.x += (m_lastCreaturePos.x - m_camera.target.x) * lerpSpeed;
    m_camera.target.y += (m_lastCreaturePos.y - m_camera.target.y) * lerpSpeed;
    m_camera.offset.x = (float)simW / 2.f;
    m_camera.offset.y = (float)(GetScreenHeight() - UI::BOTTOM_PANEL_H) / 2.f;
    
    // Zoom in dramatically
    float targetZoom = 3.5f;
    m_camera.zoom += (targetZoom - m_camera.zoom) * lerpSpeed;
    
    if (m_extinctAnimTimer >= m_extinctAnimDuration) {
        // Reset camera
        m_camera.zoom = 1.0f;
        m_camera.target = {0, 0};
        m_camera.offset = {0, 0};
        m_state = AppState::SIM_PAUSED;
    }
}

void App::drawExtinctionAnimation() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float t = m_extinctAnimTimer;
    float dur = m_extinctAnimDuration;
    float progress = t / dur;
    if (progress > 1.0f) progress = 1.0f;
    
    // Convert last creature's world position to screen position
    Vector2 screenPos = GetWorldToScreen2D(m_lastCreaturePos, m_camera);
    
    // ── Phase 1: Initial flash (0.0 - 0.3s) ──
    if (t < 0.3f) {
        float flashAlpha = (1.0f - t / 0.3f);
        DrawRectangle(0, 0, screenW, screenH, Color{255, 255, 255, (unsigned char)(flashAlpha * 200)});
    }
    
    // ── Phase 2: Expanding explosion rings ──
    int numRings = 6;
    for (int i = 0; i < numRings; i++) {
        float ringDelay = (float)i * 0.12f;
        float ringTime = t - ringDelay;
        if (ringTime < 0) continue;
        
        float ringProgress = ringTime / (dur - ringDelay);
        if (ringProgress > 1.0f) ringProgress = 1.0f;
        
        // Ease out
        float eased = 1.0f - (1.0f - ringProgress) * (1.0f - ringProgress);
        
        float maxRadius = 200.f + (float)i * 80.f;
        float radius = eased * maxRadius;
        float thickness = 4.f - ringProgress * 3.f;
        if (thickness < 1.0f) thickness = 1.0f;
        
        unsigned char ringAlpha = (unsigned char)((1.0f - ringProgress) * 255);
        
        // Use creature's color tinted with fire
        unsigned char r = (unsigned char)std::min(255, (int)m_lastCreatureColor.r + 100);
        unsigned char g = (unsigned char)std::max(0, (int)m_lastCreatureColor.g - i * 30);
        unsigned char b = (unsigned char)std::max(0, (int)m_lastCreatureColor.b - i * 40);
        
        DrawCircleLines((int)screenPos.x, (int)screenPos.y, radius, Color{r, g, b, ringAlpha});
        DrawCircleLines((int)screenPos.x, (int)screenPos.y, radius + thickness, Color{r, g, b, (unsigned char)(ringAlpha / 2)});
    }
    
    // ── Phase 3: Particle debris ──
    int numParticles = 24;
    for (int i = 0; i < numParticles; i++) {
        float angle = (float)i * (6.28318f / (float)numParticles) + t * 0.3f;
        float speed = 80.f + (float)(i % 5) * 40.f;
        float eased = 1.0f - (1.0f - progress) * (1.0f - progress);
        float dist = eased * speed * 2.5f;
        
        float px = screenPos.x + std::cos(angle) * dist;
        float py = screenPos.y + std::sin(angle) * dist;
        
        unsigned char pAlpha = (unsigned char)((1.0f - progress) * 220);
        float pSize = 3.f + 2.f * std::sin(t * 5.f + (float)i);
        if (pSize < 1.0f) pSize = 1.0f;
        
        Color pColor;
        if (i % 3 == 0) pColor = Color{255, 200, 50, pAlpha};  // gold
        else if (i % 3 == 1) pColor = Color{255, 100, 30, pAlpha}; // orange
        else pColor = Color{m_lastCreatureColor.r, m_lastCreatureColor.g, m_lastCreatureColor.b, pAlpha};
        
        DrawCircleV({px, py}, pSize, pColor);
    }
    
    // ── Phase 4: Central glow / fireball ──
    if (t < dur * 0.7f) {
        float fireProgress = t / (dur * 0.7f);
        float fireRadius = m_lastCreatureRadius * (1.0f + fireProgress * 8.f);
        unsigned char fireAlpha = (unsigned char)((1.0f - fireProgress) * 200);
        
        DrawCircleV(screenPos, fireRadius, Color{255, 150, 30, (unsigned char)(fireAlpha / 2)});
        DrawCircleV(screenPos, fireRadius * 0.6f, Color{255, 220, 100, fireAlpha});
        DrawCircleV(screenPos, fireRadius * 0.3f, Color{255, 255, 220, (unsigned char)std::min(255, (int)fireAlpha + 50)});
    }
    
    // ── Phase 5: Screen shake simulation via slight overlay jitter ──
    if (t < 1.0f) {
        float shakeIntensity = (1.0f - t) * 8.f;
        float offsetX = std::sin(t * 50.f) * shakeIntensity;
        float offsetY = std::cos(t * 37.f) * shakeIntensity;
        // Draw thin shake lines for visual impact
        DrawLineEx({screenPos.x + offsetX - 30, screenPos.y + offsetY},
                   {screenPos.x + offsetX + 30, screenPos.y + offsetY}, 1.5f, Color{255, 255, 255, (unsigned char)((1.0f - t) * 100)});
        DrawLineEx({screenPos.x + offsetX, screenPos.y + offsetY - 30},
                   {screenPos.x + offsetX, screenPos.y + offsetY + 30}, 1.5f, Color{255, 255, 255, (unsigned char)((1.0f - t) * 100)});
    }
    
    // ── Fade to dark as animation ends ──
    if (progress > 0.6f) {
        float fadeAlpha = (progress - 0.6f) / 0.4f;
        DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, (unsigned char)(fadeAlpha * 220)});
    }
    
    // ── "EXTINCTION" text fading in during last half ──
    if (progress > 0.4f) {
        float textAlpha = (progress - 0.4f) / 0.6f;
        if (textAlpha > 1.0f) textAlpha = 1.0f;
        unsigned char ta = (unsigned char)(textAlpha * 255);
        
        const char* extText = "E X T I N C T I O N";
        int extSize = 50;
        int extW = MeasureTextCustom(extText, extSize);
        
        // Red glow behind
        DrawTextCustom(extText, screenW / 2 - extW / 2 - 1, screenH / 2 - 26, extSize, Color{200, 0, 0, (unsigned char)(ta / 3)});
        DrawTextCustom(extText, screenW / 2 - extW / 2 + 1, screenH / 2 - 24, extSize, Color{200, 0, 0, (unsigned char)(ta / 3)});
        DrawTextCustom(extText, screenW / 2 - extW / 2, screenH / 2 - 25, extSize, Color{255, 60, 60, ta});
    }
}
