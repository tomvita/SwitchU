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

namespace ui {

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
    void applyTheme();
    void toggleTheme();
    void handleInput();
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

    // Touch state
    int  m_touchHitIndex = -1;   // page-local icon index under finger, or -1
    bool m_touchOnFocused = false; // finger landed on already-focused icon
};

} // namespace ui
