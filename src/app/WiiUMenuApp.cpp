#include "WiiUMenuApp.hpp"
#include "GlossyIcon.hpp"
#include "../core/Animation.hpp"
#include "../core/DebugLog.hpp"
#include <switch.h>
#include <nxtc.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ui {

WiiUMenuApp::WiiUMenuApp() {}
WiiUMenuApp::~WiiUMenuApp() { shutdown(); }

bool WiiUMenuApp::initialize() {
    DebugLog::log("[init] GPU...");
    if (!m_gpu.initialize()) {
        DebugLog::log("[init] GPU FAILED");
        return false;
    }
    m_renderer = new Renderer(m_gpu);
    DebugLog::log("[init] Renderer...");
    if (!m_renderer->initialize()) {
        DebugLog::log("[init] Renderer FAILED");
        return false;
    }

    DebugLog::log("[init] Input...");
    m_input.initialize();
    DebugLog::log("[init] Audio...");
    m_audio.initialize();
    m_audio.loadTrack("romfs:/music/bg1.mp3");
    m_audio.loadTrack("romfs:/music/bg2.mp3");
    m_audio.loadTrack("romfs:/music/bg3.mp3");
    m_audio.loadTrack("romfs:/music/bg4.mp3");
    m_audio.setVolume(0.4f);
    m_audio.loadSfx(Sfx::Navigate,    "romfs:/sfx/deck_ui_navigation.wav");
    m_audio.loadSfx(Sfx::Activate,    "romfs:/sfx/deck_ui_default_activation.wav");
    m_audio.loadSfx(Sfx::PageChange,  "romfs:/sfx/deck_ui_tab_transition_01.wav");
    m_audio.loadSfx(Sfx::ModalShow,   "romfs:/sfx/deck_ui_show_modal.wav");
    m_audio.loadSfx(Sfx::ModalHide,   "romfs:/sfx/deck_ui_hide_modal.wav");
    m_audio.loadSfx(Sfx::LaunchGame,  "romfs:/sfx/deck_ui_launch_game.wav");
    m_audio.loadSfx(Sfx::ThemeToggle, "romfs:/sfx/deck_ui_switch_toggle_on.wav");
    m_audio.setSfxVolume(0.7f);
    m_audio.play();
    DebugLog::log("[init] Audio OK, music playing");

    DebugLog::log("[init] nxtc...");
    if (!nxtcInitialize()) {
        DebugLog::log("[init] nxtc failed (non-fatal)");
    } else {
        DebugLog::log("[init] nxtc OK");
    }

    // account already initialized in __appInit (AccountServiceType_System)

    DebugLog::log("[init] loadResources...");
    loadResources();
    DebugLog::log("[init] buildGrid...");
    buildGrid();
    DebugLog::log("[init] DONE");
    return true;
}

void WiiUMenuApp::loadResources() {
    m_fontNormal.load(m_gpu, *m_renderer, "romfs:/fonts/DejaVuSans.ttf", 24);
    m_fontSmall.load(m_gpu, *m_renderer, "romfs:/fonts/DejaVuSans.ttf", 18);

    NsApplicationRecord* records = new NsApplicationRecord[1024]();
    s32 recordCount = 0;
    nsListApplicationRecord(records, 1024, 0, &recordCount);

    NsApplicationControlData controlData;

    for (int i = 0; i < recordCount; ++i) {
        uint64_t tid = records[i].application_id;

        // Try the nxtc cache first (fast path)
        NxTitleCacheApplicationMetadata* meta = nxtcGetApplicationMetadataEntryById(tid);
        if (meta) {
            std::stringstream ss;
            ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << tid;

            int texIdx = -1;
            if (meta->icon_data && meta->icon_size > 0) {
                Texture tex;
                if (tex.loadFromMemory(m_gpu, *m_renderer,
                                       static_cast<const uint8_t*>(meta->icon_data), meta->icon_size)) {
                    texIdx = (int)m_iconTextures.size();
                    m_iconTextures.push_back(std::move(tex));
                }
            }

            AppEntry entry;
            entry.id      = ss.str();
            entry.title   = meta->name ? meta->name : "";
            entry.titleId = tid;
            entry.iconTexIndex = texIdx;
            m_model.addEntry(std::move(entry));

            nxtcFreeApplicationMetadata(&meta);
            continue;
        }

        // Fallback: retrieve via NS (slow on HOS 20.0.0+)
        size_t controlSize = 0;
        Result rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, tid,
                                                &controlData, sizeof(controlData), &controlSize);
        if (R_FAILED(rc)) continue;

        NacpLanguageEntry* langEntry = nullptr;
        rc = nacpGetLanguageEntry(&controlData.nacp, &langEntry);
        if (R_FAILED(rc) || !langEntry || langEntry->name[0] == '\0') {
            langEntry = nullptr;
            for (int l = 0; l < 16; ++l) {
                NacpLanguageEntry* e = &controlData.nacp.lang[l];
                if (e->name[0] != '\0') { langEntry = e; break; }
            }
        }
        if (!langEntry || langEntry->name[0] == '\0') continue;

        size_t iconSize = controlSize - sizeof(NacpStruct);

        // Populate nxtc cache for next launch
        nxtcAddEntry(tid, &controlData.nacp, iconSize,
                     iconSize > 0 ? controlData.icon : nullptr, false);

        std::stringstream ss;
        ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << tid;

        int texIdx = -1;
        if (iconSize > 0) {
            Texture tex;
            if (tex.loadFromMemory(m_gpu, *m_renderer,
                                   controlData.icon, iconSize)) {
                texIdx = (int)m_iconTextures.size();
                m_iconTextures.push_back(std::move(tex));
            }
        }

        AppEntry entry;
        entry.id      = ss.str();
        entry.title   = langEntry->name;
        entry.titleId = tid;
        entry.iconTexIndex = texIdx;
        m_model.addEntry(std::move(entry));
    }

    // Flush any new entries to disk
    nxtcFlushCacheFile();

    delete[] records;
}

void WiiUMenuApp::buildGrid() {
    // Start with dark theme
    m_darkMode = true;
    m_theme = Theme::dark();

    // Create icons from real app data
    std::vector<std::shared_ptr<GlossyIcon>> icons;
    for (int i = 0; i < m_model.count(); ++i) {
        const auto& entry = m_model.at(i);
        auto icon = std::make_shared<GlossyIcon>();
        icon->setTitle(entry.title);
        icon->setTitleId(entry.titleId);
        if (entry.iconTexIndex >= 0 && entry.iconTexIndex < (int)m_iconTextures.size()) {
            icon->setTexture(&m_iconTextures[entry.iconTexIndex]);
        }
        icon->setCornerRadius(m_theme.iconCornerRadius);
        icons.push_back(std::move(icon));
    }

    m_background = std::make_shared<WaraWaraBackground>();
    m_background->setRect({0, 0, 1280, 720});

    m_grid = std::make_shared<IconGrid>();
    m_grid->setRect({0, 90, 1280, 540});
    m_grid->setup(std::move(icons), 5, 3, 150, 150, 20, 16);

    m_cursor = std::make_shared<SelectionCursor>();

    m_clock = std::make_shared<DateTimeWidget>();
    m_clock->setRect({24, 14, 150, 62});
    m_clock->setFont(&m_fontNormal);
    m_clock->setSmallFont(&m_fontSmall);
    m_clock->setCornerRadius(m_theme.cellCornerRadius);

    m_battery = std::make_shared<BatteryWidget>();
    m_battery->setRect({1106, 14, 150, 62});
    m_battery->setFont(&m_fontSmall);
    m_battery->setCornerRadius(m_theme.cellCornerRadius);

    m_titlePill = std::make_shared<TitlePillWidget>();
    m_titlePill->setPosition(0, 630.f);
    m_titlePill->setFont(&m_fontNormal);
    m_titlePill->setPadding(9.f, 22.f, 9.f, 22.f);

    m_launchAnim = std::make_shared<LaunchAnimation>();

    m_userSelect = std::make_shared<UserSelectScreen>();
    m_userSelect->setFont(&m_fontNormal);
    m_userSelect->setSmallFont(&m_fontSmall);
    m_userSelect->setAudio(&m_audio);
    m_userSelect->loadUsers(m_gpu, *m_renderer);

    m_grid->focusManager().onFocusChanged([this](IFocusable*, IFocusable*) {
        updateCursor();
        m_audio.playSfx(Sfx::Navigate);
        auto* cur = m_grid->focusManager().current();
        if (cur) {
            auto* icon = static_cast<GlossyIcon*>(cur);
            if (isAppSuspended(icon->titleId())) {
                m_titlePill->setText(std::string("\xe2\x96\xb6  ") + icon->title());
            } else {
                m_titlePill->setText(icon->title());
            }
            m_titlePill->setVisible(true);
        } else {
            m_titlePill->setVisible(false);
        }
    });
    updateCursor();
    // Set initial title pill
    if (auto* cur = m_grid->focusManager().current()) {
        auto* icon = static_cast<GlossyIcon*>(cur);
        m_titlePill->setText(icon->title());
    }
    m_grid->startAppearAnimation();
    applyTheme();
}

void WiiUMenuApp::refreshAppList() {
    DebugLog::log("[refresh] re-fetching app list...");

    // Remember which page we were on
    int prevPage = m_grid ? m_grid->currentPage() : 0;

    // Clear old data
    m_model.clear();
    m_iconTextures.clear();

    // Re-fetch from NS + nxtc
    loadResources();
    DebugLog::log("[refresh] found %d apps", m_model.count());

    // Rebuild icon widgets
    std::vector<std::shared_ptr<GlossyIcon>> icons;
    for (int i = 0; i < m_model.count(); ++i) {
        const auto& entry = m_model.at(i);
        auto icon = std::make_shared<GlossyIcon>();
        icon->setTitle(entry.title);
        icon->setTitleId(entry.titleId);
        if (entry.iconTexIndex >= 0 && entry.iconTexIndex < (int)m_iconTextures.size()) {
            icon->setTexture(&m_iconTextures[entry.iconTexIndex]);
        }
        icon->setCornerRadius(m_theme.iconCornerRadius);
        icon->setBaseColor(m_theme.panelBase);
        // Restore suspended indicator if this app is still suspended
        if (m_suspendedTitleId != 0 && entry.titleId == m_suspendedTitleId)
            icon->setSuspended(true);
        icons.push_back(std::move(icon));
    }

    // Feed new icons into existing grid
    m_grid->setup(std::move(icons), 5, 3, 150, 150, 20, 16);

    // Restore page (clamped to valid range)
    if (prevPage > 0)
        m_grid->setPage(prevPage);

    // Re-wire focus callback
    m_grid->focusManager().onFocusChanged([this](IFocusable*, IFocusable*) {
        updateCursor();
        m_audio.playSfx(Sfx::Navigate);
        auto* cur = m_grid->focusManager().current();
        if (cur) {
            auto* icon = static_cast<GlossyIcon*>(cur);
            if (isAppSuspended(icon->titleId())) {
                m_titlePill->setText(std::string("\xe2\x96\xb6  ") + icon->title());
            } else {
                m_titlePill->setText(icon->title());
            }
            m_titlePill->setVisible(true);
        } else {
            m_titlePill->setVisible(false);
        }
    });

    updateCursor();

    // Update title pill for current focus
    if (auto* cur = m_grid->focusManager().current()) {
        auto* icon = static_cast<GlossyIcon*>(cur);
        if (isAppSuspended(icon->titleId())) {
            m_titlePill->setText(std::string("\xe2\x96\xb6  ") + icon->title());
        } else {
            m_titlePill->setText(icon->title());
        }
    }

    m_grid->startAppearAnimation();

    DebugLog::log("[refresh] done, %d icons on page %d", m_model.count(), m_grid->currentPage());
}

void WiiUMenuApp::applyTheme() {
    m_background->setAccentColor(m_theme.backgroundAccent);
    m_background->setSecondaryColor(m_theme.background);
    m_background->setShapeColor(m_theme.shapeColor);

    for (auto& icon : m_grid->allIcons()) {
        icon->setBaseColor(m_theme.panelBase);
        icon->setCornerRadius(m_theme.iconCornerRadius);
    }

    m_cursor->setColor(m_theme.cursorNormal);
    m_cursor->setCornerRadius(m_theme.cursorCornerRadius);
    m_cursor->setBorderWidth(m_theme.cursorBorderWidth);

    m_clock->setBaseColor(m_theme.panelBase);
    m_clock->setBorderColor(m_theme.panelBorder);
    m_clock->setHighlightColor(m_theme.panelHighlight);
    m_clock->setTextColor(m_theme.textPrimary);
    m_clock->setSecondaryTextColor(m_theme.textSecondary);

    m_battery->setBaseColor(m_theme.panelBase);
    m_battery->setBorderColor(m_theme.panelBorder);
    m_battery->setHighlightColor(m_theme.panelHighlight);
    m_battery->setTextColor(m_theme.textPrimary);

    m_titlePill->setBaseColor(m_theme.panelBase);
    m_titlePill->setBorderColor(m_theme.panelBorder);
    m_titlePill->setHighlightColor(m_theme.panelHighlight);
    m_titlePill->setTextColor(m_theme.textPrimary);

    m_userSelect->panel().setBaseColor(m_theme.panelBase);
    m_userSelect->panel().setBorderColor(m_theme.panelBorder);
    m_userSelect->panel().setHighlightColor(m_theme.panelHighlight);
    m_userSelect->panel().setPanelOpacity(1.5f);
    m_userSelect->titlePanel().setBaseColor(m_theme.panelBase);
    m_userSelect->titlePanel().setBorderColor(m_theme.panelBorder);
    m_userSelect->titlePanel().setHighlightColor(m_theme.panelHighlight);
    m_userSelect->titlePanel().setPanelOpacity(1.5f);
    m_userSelect->setTextColor(m_theme.textPrimary);
    m_userSelect->setSecondaryTextColor(m_theme.textSecondary);
    m_userSelect->cursor().setColor(m_theme.cursorNormal);
}

void WiiUMenuApp::toggleTheme() {
    m_darkMode = !m_darkMode;
    m_theme = m_darkMode ? Theme::dark() : Theme::light();
    applyTheme();
    m_audio.playSfx(Sfx::ThemeToggle);
}

void WiiUMenuApp::run() {
    uint64_t prevTick = armGetSystemTick();
    int frameCount = 0;
    DebugLog::log("[run] starting message threads");
    startMessageThreads();
    DebugLog::log("[run] entering main loop");
    // qlaunch must NEVER exit if AM detects it has stopped, the system crashes.
    while (m_running) {
        pumpSystemMessages();   // drains thread-queued actions (fast, no IPC)
        checkRunningApplication();

        uint64_t nowTick = armGetSystemTick();
        float dt = (float)(nowTick - prevTick) / armGetSystemTickFreq();
        prevTick = nowTick;
        if (dt > 0.1f) dt = 0.016f;

        handleInput();
        update(dt);
        render();

        frameCount++;
        if (frameCount % 60 == 0) {
            DebugLog::log("[run] frame %d  dt=%.3f", frameCount, dt);
        }
    }
    DebugLog::log("[run] EXITED main loop!");
    stopMessageThreads();
}

void WiiUMenuApp::pumpSystemMessages() {
    // Drain UI-state actions queued by the background threads.
    // The actual applet IPC (RequestToGetForeground, StartSleepSequence, .....)
    // is done directly from the threads matching DeltaLaunch's pattern.
    std::vector<SysAction> actions;
    {
        std::lock_guard<std::mutex> lk(m_actionMutex);
        actions.swap(m_pendingActions);
    }

    for (auto a : actions) {
        switch (a) {
            case SysAction::HomeButton:
                DebugLog::log("[pump] HomeButton -> UI update");
                if (m_appRunning) {
                    m_appHasForeground = false;
                    m_showLoadingScreen = false;
                    DebugLog::log("[pump] foreground reclaimed from app");
                }

                // Re-fetch full app list (order changes, installs/uninstalls)
                refreshAppList();
                break;
            default:
                break;
        }
    }
}

// Background thread: Applet messages (ICommonStateGetter::ReceiveMessage)
// Matches DeltaLaunch's AeMessageThread pattern exactly:
//   tight poll on appletGetMessage() + yield, call applet IPC directly.
void WiiUMenuApp::aeThreadFunc(void* arg) {
    auto* self = static_cast<WiiUMenuApp*>(arg);

    while (self->m_threadsRunning) {
        u32 msg = 0;
        if (R_SUCCEEDED(appletGetMessage(&msg))) {
            DebugLog::log("[ae] applet msg=%u", msg);
            switch (msg) {
                case 20: // DetectShortPressingHomeButton
                    DebugLog::log("[ae] -> Home");
                    appletRequestToGetForeground();
                    self->pushAction(SysAction::HomeButton);
                    break;
                case 22: // DetectShortPressingPowerButton
                case 29: // AutoPowerDown
                case 32: // (additional power variant)
                    DebugLog::log("[ae] -> Sleep (msg=%u)", msg);
                    appletStartSleepSequence(true);
                    break;
                case 26: // Wakeup
                    DebugLog::log("[ae] -> Wakeup");
                    break;
                case AppletMessage_OperationModeChanged:
                case AppletMessage_PerformanceModeChanged:
                    DebugLog::log("[ae] -> mode change");
                    break;
                default:
                    DebugLog::log("[ae] unhandled msg=%u", msg);
                    break;
            }
        }
        svcSleepThread(0);
    }
}

// Background thread: General-channel / SAMS messages
//   get event -> eventWait -> pop one message -> close event -> loop.
//   Applet IPC is called directly from this thread.
void WiiUMenuApp::samsThreadFunc(void* arg) {
    auto* self = static_cast<WiiUMenuApp*>(arg);

    while (self->m_threadsRunning) {
        // DeltaLaunch creates a fresh event handle every iteration.
        Event epop;
        Result rc = appletGetPopFromGeneralChannelEvent(&epop);
        if (R_FAILED(rc)) {
            DebugLog::log("[sams] GetGCEvent FAIL: 0x%X", rc);
            svcSleepThread(1'000'000'000ULL); // 1 s back-off
            continue;
        }

        // Block until a general-channel storage is available.
        rc = eventWait(&epop, 1'000'000'000ULL); // 1 s timeout (check exit flag)
        if (R_SUCCEEDED(rc)) {
            AppletStorage st;
            rc = appletPopFromGeneralChannel(&st);
            if (R_SUCCEEDED(rc)) {
                struct SamsHeader {
                    u32 magic;
                    u32 version;
                    u32 msg;
                    u32 reserved;
                } hdr = {};

                s64 stSize = 0;
                appletStorageGetSize(&st, &stSize);
                if (stSize > 0) {
                    appletStorageRead(&st, 0, &hdr,
                        (size_t)stSize < sizeof(hdr) ? (size_t)stSize : sizeof(hdr));
                }

                if (hdr.magic == 0x534D4153 /* 'SAMS' */) {
                    DebugLog::log("[sams] msg=%u", hdr.msg);
                    switch (hdr.msg) {
                        case 2: // RequestHomeMenu
                            DebugLog::log("[sams] -> Home");
                            appletRequestToGetForeground();
                            self->pushAction(SysAction::HomeButton);
                            break;
                        case 3: // Sleep
                            DebugLog::log("[sams] -> Sleep");
                            appletStartSleepSequence(true);
                            break;
                        case 5: // Shutdown
                            DebugLog::log("[sams] -> Shutdown");
                            appletStartShutdownSequence();
                            break;
                        case 6: // Reboot
                            DebugLog::log("[sams] -> Reboot");
                            appletStartRebootSequence();
                            break;
                        default:
                            DebugLog::log("[sams] unhandled msg=%u", hdr.msg);
                            break;
                    }
                } else {
                    DebugLog::log("[sams] non-SAMS sz=%lld magic=0x%08X",
                                  (long long)stSize, hdr.magic);
                }

                appletStorageClose(&st);
            }
        }

        eventClose(&epop);
    }
}

void WiiUMenuApp::startMessageThreads() {
    m_threadsRunning = true;

    // DeltaLaunch uses: stack 0x20000, priority 0x3B, core (current+1)%4
    s32 msgCore = (svcGetCurrentProcessorNumber() + 1) % 4;

    Result rc = threadCreate(&m_samsThread, samsThreadFunc, this,
                             nullptr, 0x20000, 0x3B, msgCore);
    if (R_SUCCEEDED(rc)) {
        threadStart(&m_samsThread);
        DebugLog::log("[threads] SAMS thread started (core %d)", msgCore);
    } else {
        DebugLog::log("[threads] SAMS thread create FAIL: 0x%X", rc);
    }

    rc = threadCreate(&m_aeThread, aeThreadFunc, this,
                      nullptr, 0x20000, 0x3B, msgCore);
    if (R_SUCCEEDED(rc)) {
        threadStart(&m_aeThread);
        DebugLog::log("[threads] AE thread started (core %d)", msgCore);
    } else {
        DebugLog::log("[threads] AE thread create FAIL: 0x%X", rc);
    }
}

void WiiUMenuApp::stopMessageThreads() {
    m_threadsRunning = false;
    threadWaitForExit(&m_samsThread);
    threadClose(&m_samsThread);
    threadWaitForExit(&m_aeThread);
    threadClose(&m_aeThread);
    DebugLog::log("[threads] message threads stopped");
}

void WiiUMenuApp::handleInput() {
    m_input.update();
    if (m_launchAnim->isPlaying()) return;

    // When user-select overlay is active, route input there exclusively
    if (m_userSelect->isActive()) {
        m_userSelect->handleInput(m_input);
        return;
    }

    if (m_input.isDown(Button::DLeft)  || m_input.isDown(Button::LStickL))
        m_grid->focusManager().moveLeft();
    if (m_input.isDown(Button::DRight) || m_input.isDown(Button::LStickR))
        m_grid->focusManager().moveRight();
    if (m_input.isDown(Button::DUp)    || m_input.isDown(Button::LStickU))
        m_grid->focusManager().moveUp();
    if (m_input.isDown(Button::DDown)  || m_input.isDown(Button::LStickD))
        m_grid->focusManager().moveDown();

    if (m_input.isDown(Button::L)) {
        int p = m_grid->currentPage() - 1;
        if (p >= 0) {
            m_grid->setPage(p);
            m_grid->startAppearAnimation();
            updateCursor();
            m_audio.playSfx(Sfx::PageChange);
        }
    }
    if (m_input.isDown(Button::R)) {
        int p = m_grid->currentPage() + 1;
        if (p < m_grid->totalPages()) {
            m_grid->setPage(p);
            m_grid->startAppearAnimation();
            updateCursor();
            m_audio.playSfx(Sfx::PageChange);
        }
    }

    if (m_input.isDown(Button::A)) {
        auto* foc = m_grid->focusManager().current();
        if (foc) {
            GlossyIcon* icon = nullptr;
            for (auto& ic : m_grid->allIcons()) {
                if (ic.get() == static_cast<GlossyIcon*>(foc)) {
                    icon = ic.get();
                    break;
                }
            }
            if (icon) {
                uint64_t tid = icon->titleId();

                // If this is the suspended app → resume it directly (no user select)
                if (isAppSuspended(tid)) {
                    m_audio.playSfx(Sfx::LaunchGame);
                    resumeApplication();
                } else {
                    m_audio.playSfx(Sfx::Activate);
                    Rect   focRect = foc->getFocusRect();
                    const Texture* tex = icon->texture();
                    float  cr      = icon->cornerRadius();
                    Color  base    = m_theme.panelBase;
                    Color  border  = m_theme.panelBorder;

                    m_userSelect->show(
                        [this, focRect, tex, cr, base, border, tid](AccountUid uid) {
                            m_audio.playSfx(Sfx::LaunchGame);
                            m_launchAnim->start(focRect, tex, cr, base, border, tid, uid,
                                [this](uint64_t id, AccountUid u) { launchApplication(id, u); });
                        });
                }
            }
        }
    }

    constexpr float kTapThreshold   = 20.f;
    constexpr float kSwipeThreshold = 80.f;

    // Finger just landed – hit-test and give visual feedback
    if (m_input.touchDown()) {
        float tx = m_input.touchX();
        float ty = m_input.touchY();
        int hit = m_grid->hitTest(tx, ty);
        m_touchHitIndex = hit;
        // Check if the finger landed on the icon that's already focused
        m_touchOnFocused = (hit >= 0 && hit == m_grid->focusManager().focusIndex());
    }

    // Finger lifted
    if (m_input.touchUp()) {
        float dx = m_input.touchDeltaX();
        float dy = m_input.touchDeltaY();
        float dist2 = dx * dx + dy * dy;

        if (dist2 < kTapThreshold * kTapThreshold && m_touchHitIndex >= 0) {
            if (m_touchOnFocused) {
                // Second tap on already-focused icon → confirm
                auto* foc = m_grid->focusManager().current();
                if (foc) {
                    GlossyIcon* icon = nullptr;
                    for (auto& ic : m_grid->allIcons()) {
                        if (ic.get() == static_cast<GlossyIcon*>(foc)) {
                            icon = ic.get();
                            break;
                        }
                    }
                    if (icon) {
                        uint64_t tid = icon->titleId();

                        // If this is the suspended app → resume directly
                        if (isAppSuspended(tid)) {
                            m_audio.playSfx(Sfx::LaunchGame);
                            resumeApplication();
                        } else {
                            m_audio.playSfx(Sfx::Activate);
                            Rect   focRect = foc->getFocusRect();
                            const Texture* tex = icon->texture();
                            float  cr      = icon->cornerRadius();
                            Color  base    = m_theme.panelBase;
                            Color  border  = m_theme.panelBorder;

                            m_userSelect->show(
                                [this, focRect, tex, cr, base, border, tid](AccountUid uid) {
                                    m_audio.playSfx(Sfx::LaunchGame);
                                    m_launchAnim->start(focRect, tex, cr, base, border, tid, uid,
                                        [this](uint64_t id, AccountUid u) { launchApplication(id, u); });
                                });
                        }
                    }
                }
            } else {
                // First tap on a different icon → just move the cursor there
                m_grid->focusManager().setFocus(m_touchHitIndex);
                updateCursor();
            }
        } else if (std::abs(dx) > kSwipeThreshold && std::abs(dx) > std::abs(dy) * 1.5f) {
            // Horizontal swipe – change page
            int p = m_grid->currentPage() + (dx < 0 ? 1 : -1);
            if (p >= 0 && p < m_grid->totalPages()) {
                m_grid->setPage(p);
                m_grid->startAppearAnimation();
                updateCursor();
                m_audio.playSfx(Sfx::PageChange);
            }
        }
        m_touchHitIndex = -1;
    }

    if (m_input.isDown(Button::X)) toggleTheme();
    if (m_input.isDown(Button::Y)) {
        m_audio.nextTrack();
        m_audio.playSfx(Sfx::PageChange);
    }
    if (m_input.isDown(Button::Minus)) {
        m_showDebugOverlay = !m_showDebugOverlay;
    }
    if (m_input.isDown(Button::Plus)) {
        m_audio.playSfx(Sfx::ModalHide);
        m_running = false;
    }
}
void WiiUMenuApp::update(float dt) {
    AnimationManager::instance().update(dt);
    m_background->update(dt);
    m_grid->update(dt);
    m_cursor->update(dt);
    m_clock->update(dt);
    m_battery->update(dt);
    m_titlePill->update(dt);
    m_launchAnim->update(dt);
    m_userSelect->update(dt);
}

void WiiUMenuApp::render() {
    m_gpu.beginFrame();
    m_renderer->beginFrame();

    if (m_showLoadingScreen && !m_launchAnim->isPlaying()) {
        // Nintendo-style loading screen: black + "Nintendo Switch" bottom-left
        m_renderer->drawRect({0, 0, 1280, 720}, Color(0, 0, 0, 1.f));
    } else {
        m_background->render(*m_renderer);
        m_grid->render(*m_renderer);

        // Touch-press highlight: subtle overlay on the touched (non-focused) icon
        if (m_touchHitIndex >= 0 && !m_touchOnFocused && m_input.isTouching()) {
            auto icons = m_grid->pageIcons();
            if (m_touchHitIndex < (int)icons.size()) {
                Rect r = icons[m_touchHitIndex]->getFocusRect();
                float cr = icons[m_touchHitIndex]->cornerRadius();
                m_renderer->drawRoundedRect(r, Color(1.f, 1.f, 1.f, 0.18f), cr);
            }
        }

        m_cursor->render(*m_renderer);
        m_clock->render(*m_renderer);
        m_battery->render(*m_renderer);
        m_titlePill->render(*m_renderer);
        drawPageIndicator();
        m_userSelect->render(*m_renderer);
        m_launchAnim->render(*m_renderer);
    }

    if (m_showDebugOverlay) {
        // Semi-transparent background for readability
        Rect logBg = {0, 0, 500, 720};
        m_renderer->drawRect(logBg, Color(0, 0, 0, 0.75f));

        auto& lines = DebugLog::lines();
        float y = 8.f;
        for (auto& line : lines) {
            m_renderer->drawText(line, {8.f, y}, &m_fontSmall, Color(0.f, 1.f, 0.f, 1.f), 1.f);
            y += 22.f;
            if (y > 700.f) break;
        }
    }

    m_renderer->endFrame();
    m_gpu.endFrame();
}

void WiiUMenuApp::updateCursor() {
    auto* cur = m_grid->focusManager().current();
    if (cur) {
        Rect fr = cur->getFocusRect();
        m_cursor->moveTo(fr.expanded(4.f));
    }
}

void WiiUMenuApp::drawFocusedTitle() {
    // Now handled by m_titlePill (GlassWidget)
}

void WiiUMenuApp::drawPageIndicator() {
    int total = m_grid->totalPages();
    if (total <= 1) return;
    int cur = m_grid->currentPage();

    float dotR    = 4.f;
    float gap     = 14.f;
    float padX    = 18.f;
    float padY    = 10.f;
    float dotsW   = total * dotR * 2.f + (total - 1) * gap;
    float pillW   = dotsW + padX * 2.f;
    float pillH   = dotR * 2.f + padY * 2.f;
    float pillX   = (1280.f - pillW) * 0.5f;
    float pillY   = 685.f;
    float pillR   = pillH * 0.5f;  // fully rounded capsule

    // Glass pill background
    Rect pill = {pillX, pillY, pillW, pillH};
    m_renderer->drawRoundedRect(pill, m_theme.panelBase, pillR);
    // Top highlight shine
    Rect shine = {pillX, pillY, pillW, pillH * 0.45f};
    m_renderer->drawRoundedRect(shine, Color(1, 1, 1, 0.06f), pillR);
    // Border
    m_renderer->drawRoundedRectOutline(pill, m_theme.panelBorder, pillR, 1.f);

    // Dots
    float dotsStartX = pillX + padX;
    float dotCY      = pillY + pillH * 0.5f;
    for (int i = 0; i < total; ++i) {
        float cx = dotsStartX + i * (dotR * 2.f + gap) + dotR;
        Color c  = (i == cur) ? m_theme.pageIndicatorActive : m_theme.pageIndicator;
        m_renderer->drawCircle({cx, dotCY}, dotR, c, 12);
    }
}

void WiiUMenuApp::launchApplication(uint64_t titleId, AccountUid uid) {
    DebugLog::log("[launch] tid=%016lX", titleId);

    // If there's already a running app, terminate it first
    if (m_appRunning) {
        DebugLog::log("[launch] closing previous app");
        appletApplicationRequestExit(&m_currentApp);
        appletApplicationJoin(&m_currentApp);
        appletApplicationClose(&m_currentApp);
        m_appRunning = false;
    }

    // 1. Create application accessor (SystemApplet-only API)
    Result rc = appletCreateApplication(&m_currentApp, titleId);
    if (R_FAILED(rc)) {
        DebugLog::log("[launch] CreateApp FAIL: 0x%X", rc);
        return;
    }
    DebugLog::log("[launch] accessor created");

    // 2. Push preselected user so the game skips its own user selector
    {
        struct {
            u32 magic;        // 0xC79497CA
            u8  is_selected;  // 1
            u8  pad[3];
            AccountUid uid;
            u8  unused[0x70];
        } userArg = {};
        static_assert(sizeof(userArg) == 0x88, "PreselectedUser struct size mismatch");

        userArg.magic       = 0xC79497CA;
        userArg.is_selected = 1;
        userArg.uid         = uid;

        AppletStorage st;
        rc = appletCreateStorage(&st, sizeof(userArg));
        if (R_SUCCEEDED(rc)) {
            appletStorageWrite(&st, 0, &userArg, sizeof(userArg));
            rc = appletApplicationPushLaunchParameter(&m_currentApp,
                AppletLaunchParameterKind_PreselectedUser, &st);
            if (R_FAILED(rc)) {
                DebugLog::log("[launch] PushUser FAIL: 0x%X", rc);
                appletStorageClose(&st);
            } else {
                DebugLog::log("[launch] user param pushed");
            }
        }
    }

    // 3. Unlock foreground so the launched app can take over the display.
    appletUnlockForeground();

    // 4. Start the application
    rc = appletApplicationStart(&m_currentApp);
    if (R_FAILED(rc)) {
        DebugLog::log("[launch] Start FAIL: 0x%X", rc);
        appletApplicationClose(&m_currentApp);
        return;
    }
    DebugLog::log("[launch] started");

    // 5. Give the application foreground
    rc = appletApplicationRequestForApplicationToGetForeground(&m_currentApp);
    if (R_FAILED(rc))
        DebugLog::log("[launch] ReqFG FAIL: 0x%X (non-fatal)", rc);

    m_appRunning = true;
    m_appHasForeground = true;
    m_showLoadingScreen = true;
    m_suspendedTitleId = titleId;
    DebugLog::log("[launch] app running OK");
}

void WiiUMenuApp::resumeApplication() {
    if (!m_appRunning) {
        DebugLog::log("[resume] no app running!");
        return;
    }
    DebugLog::log("[resume] giving app foreground back");

    // Clear suspended indicators
    for (auto& ic : m_grid->allIcons())
        ic->setSuspended(false);

    appletUnlockForeground();
    Result rc = appletApplicationRequestForApplicationToGetForeground(&m_currentApp);
    if (R_FAILED(rc))
        DebugLog::log("[resume] ReqFG FAIL: 0x%X", rc);

    m_appHasForeground = true;
    m_showLoadingScreen = true;
}

bool WiiUMenuApp::isAppSuspended(uint64_t titleId) const {
    return m_appRunning && !m_appHasForeground && m_suspendedTitleId == titleId;
}

void WiiUMenuApp::checkRunningApplication() {
    if (!m_appRunning) return;

    if (appletApplicationCheckFinished(&m_currentApp)) {
        DebugLog::log("[app] application finished");
        appletApplicationJoin(&m_currentApp);
        auto reason = appletApplicationGetExitReason(&m_currentApp);
        DebugLog::log("[app] exit reason: %d", (int)reason);
        appletApplicationClose(&m_currentApp);
        m_appRunning = false;
        m_appHasForeground = false;
        m_showLoadingScreen = false;
        m_suspendedTitleId = 0;

        // Mark all icons as not suspended
        for (auto& ic : m_grid->allIcons())
            ic->setSuspended(false);

        // Foreground returns to us automatically; request just in case
        appletRequestToGetForeground();
        DebugLog::log("[app] foreground reclaimed");
    }
}

void WiiUMenuApp::shutdown() {
    if (m_appRunning) {
        appletApplicationRequestExit(&m_currentApp);
        appletApplicationJoin(&m_currentApp);
        appletApplicationClose(&m_currentApp);
        m_appRunning = false;
    }
    // Service exits (account, ns, applet, …) are handled by __appExit
    nxtcExit();
    m_audio.shutdown();
    delete m_renderer;
    m_renderer = nullptr;
    m_gpu.shutdown();
}

} // namespace ui
