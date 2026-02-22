#pragma once
#include "../core/GpuDevice.hpp"
#include "../core/Renderer.hpp"
#include "../core/Input.hpp"
#include "../core/Font.hpp"
#include "../core/Texture.hpp"
#include "../ui/IconGrid.hpp"
#include "../ui/Theme.hpp"
#include "GridModel.hpp"
#include "SelectionCursor.hpp"
#include "WaraWaraBackground.hpp"
#include "DateTimeWidget.hpp"
#include "BatteryWidget.hpp"
#include "TitlePillWidget.hpp"
#include "AudioManager.hpp"
#include "LaunchAnimation.hpp"
#include "UserSelectScreen.hpp"
#include "../ui/Background.hpp"
#include <memory>
#include <vector>
#include <mutex>
#include <switch.h>

namespace ui {

// Messages that background threads push for the main loop to handle.
enum class SysAction {
    HomeButton,         // Home pressed or SAMS RequestHomeMenu
    Sleep,              // Power button or SAMS Sleep
    Shutdown,           // SAMS Shutdown
    Reboot,             // SAMS Reboot
    OperationModeChanged,
};

class WiiUMenuApp {
public:
    WiiUMenuApp();
    ~WiiUMenuApp();

    bool initialize();
    void run();
    void shutdown();

private:
    void loadResources();
    void buildGrid();
    void refreshAppList();   // re-fetch apps + covers on Home return
    void applyTheme();
    void toggleTheme();
    void handleInput();
    void pumpSystemMessages();       // drains m_pendingActions from threads
    void startMessageThreads();
    void stopMessageThreads();
    static void samsThreadFunc(void* arg);   // general-channel thread
    static void aeThreadFunc(void* arg);     // applet-message thread
    void launchApplication(uint64_t titleId, AccountUid uid);
    void resumeApplication();          // give suspended app foreground back
    void checkRunningApplication();
    bool isAppSuspended(uint64_t titleId) const;
    void update(float dt);
    void render();
    void updateCursor();
    void drawPageIndicator();
    void drawFocusedTitle();

    GpuDevice  m_gpu;
    Renderer*  m_renderer = nullptr;
    Input      m_input;

    Font  m_fontNormal;
    Font  m_fontSmall;
    std::vector<Texture> m_iconTextures;

    GridModel    m_model;
    Theme        m_theme;
    bool         m_darkMode = true;

    std::shared_ptr<Background>         m_background;
    std::shared_ptr<IconGrid>           m_grid;
    std::shared_ptr<SelectionCursor>    m_cursor;
    std::shared_ptr<DateTimeWidget>     m_clock;
    std::shared_ptr<BatteryWidget>      m_battery;
    std::shared_ptr<TitlePillWidget>    m_titlePill;
    std::shared_ptr<LaunchAnimation>    m_launchAnim;
    std::shared_ptr<UserSelectScreen>    m_userSelect;

    AudioManager m_audio;

    bool m_running = true;

    // Debug overlay
    bool m_showDebugOverlay = false;

    // Nintendo-style loading screen while game starts
    bool m_showLoadingScreen = false;

    // Launched application tracking
    AppletApplication m_currentApp = {};
    bool m_appRunning = false;
    bool m_appHasForeground = false;  // true when launched app has display focus
    uint64_t m_suspendedTitleId = 0;  // titleId of currently suspended app (0 if none)

    // Touch state
    int  m_touchHitIndex = -1;   // page-local icon index under finger, or -1
    bool m_touchOnFocused = false; // finger landed on already-focused icon

    // Message threads (DeltaLaunch pattern)
    Thread m_samsThread = {};     // general-channel / SAMS messages
    Thread m_aeThread   = {};     // applet (ICommonStateGetter) messages
    bool   m_threadsRunning = false;

    // Thread-safe action queue
    std::mutex              m_actionMutex;
    std::vector<SysAction>  m_pendingActions;

    void pushAction(SysAction a) {
        std::lock_guard<std::mutex> lk(m_actionMutex);
        m_pendingActions.push_back(a);
    }
};

} // namespace ui
