#pragma once
#include "raylib.h"
#include "Simulation.h"
#include "UI.h"
#include <vector>
#include <string>
#include <filesystem>

enum class AppState {
    SPLASH_SCREEN,
    STARTUP_MENU,
    SIM_RUNNING,
    SIM_PAUSED,
    OPTIONS_MENU,
    STATS_MENU,
    LOADING_SCREEN,
    EXTINCTION_ANIMATION
};

struct AppStats {
    long long lifetimeBirths = 0;
    long long lifetimeDeaths = 0;
    long long lifetimeDisasters = 0;
    int highestPopulation = 0;
    float highestSpeed = 0.0f;
    float highestVision = 0.0f;
};

class App {
public:
    App();
    ~App();

    void run();

private:
    void loadResources();
    void loadStats();
    void saveStats();

    void updateMusicStream();
    void playRandomMenuBGM();
    void playRandomSimBGM();
    void loadBackground(int index);
    void loadClickSound(int index);

    void drawSplashScreen();
    void drawStartupMenu();
    void drawSimRunning();
    void drawSimPaused();
    void drawOptionsMenu();
    void drawStatsMenu();
    void drawLoadingScreen();
    void drawExtinctionAnimation();

    void updateSplashScreen();
    void updateStartupMenu();
    void updateSimRunning();
    void updateSimPaused();
    void updateOptionsMenu();
    void updateStatsMenu();
    void updateLoadingScreen();
    void updateExtinctionAnimation();

    // Core
    AppState m_state;
    AppState m_previousState; // to go back from Options/Stats
    Simulation m_sim;
    UI m_ui;
    
    // Window Mode
    bool m_isBorderlessFullscreen;

    // Simulation specific UI state from main.cpp
    PlayMode m_playMode;
    int m_selectedCreatureId;
    const Creature* m_selectedCreature;
    std::string m_statusMsg;
    int m_statusTimer;
    void showStatus(const std::string& msg);

    // Camera
    Camera2D m_camera;
    bool m_isTrackingCreature;
    const float TRACK_ZOOM_TARGET = 2.5f;
    
    // Disaster input state
    enum class InputMode { Normal, PlacingMeteor, PlacingNuke };
    InputMode m_inputMode;
    std::vector<Vector2> m_meteorTargets;

    // Audio / Assets
    std::vector<std::string> m_menuBgmFiles;
    std::vector<std::string> m_simBgmFiles;
    std::vector<std::string> m_bgFiles;
    std::vector<std::string> m_clickSoundFiles;
    
    Music m_currentMenuBgm;
    Music m_currentSimBgm;
    Sound m_clickSound;
    Texture2D m_currentBg;
    Image m_currentBgImage;
    bool m_isBgAnimated;
    int m_bgAnimFrames;
    int m_bgCurrentFrame;
    int m_bgFrameDelay;
    int m_bgFrameCounter;
    
    int m_activeBgIndex;
    int m_activeClickSoundIndex;
    float m_bgmVolume; // 0.0f to 1.0f

    // Persistent stats
    AppStats m_appStats;
    bool m_isExtinct;
    float m_extinctionRuntime;

    // ── Splash Screen ──
    float m_splashTimer;
    float m_splashDuration;

    // ── Loading Screen ──
    float m_loadingTimer;
    float m_loadingDuration;
    float m_loadingZoomPhase; // for camera zoom-out animation after load
    bool  m_loadingDone;      // progress bar complete, playing zoom anim

    // ── Extinction Animation ──
    float   m_extinctAnimTimer;
    float   m_extinctAnimDuration;
    Vector2 m_lastCreaturePos;
    Color   m_lastCreatureColor;
    float   m_lastCreatureRadius;
    
    // GUI Helpers for buttons
    bool DrawButton(const char* text, int x, int y, int width, int height);
};
