#include "WiiUMenuApp.hpp"
#include "GlossyIcon.hpp"
#include "../core/Animation.hpp"
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
    if (!m_gpu.initialize()) {
        std::printf("GPU init failed\n");
        return false;
    }
    m_renderer = new Renderer(m_gpu);
    if (!m_renderer->initialize()) {
        std::printf("Renderer init failed\n");
        return false;
    }

    m_input.initialize();
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

    if (!nxtcInitialize()) {
        std::printf("nxtc init failed (non-fatal, will use NS fallback)\n");
    }

    // Try Administrator first (works in applet mode), fall back to System, then Application
    Result accRc = accountInitialize(AccountServiceType_Administrator);
    if (R_FAILED(accRc))
        accRc = accountInitialize(AccountServiceType_System);
    if (R_FAILED(accRc))
        accountInitialize(AccountServiceType_Application);

    loadResources();
    buildGrid();
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
    m_grid->setup(std::move(icons), 5, 3, 130, 130, 28, 24);

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
            m_titlePill->setText(icon->title());
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
    while (appletMainLoop() && m_running) {
        uint64_t nowTick = armGetSystemTick();
        float dt = (float)(nowTick - prevTick) / armGetSystemTickFreq();
        prevTick = nowTick;
        if (dt > 0.1f) dt = 0.016f;

        handleInput();
        update(dt);
        render();
    }
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
                m_audio.playSfx(Sfx::Activate);
                // Capture launch info for callback
                Rect   focRect = foc->getFocusRect();
                const Texture* tex = icon->texture();
                float  cr      = icon->cornerRadius();
                uint64_t tid   = icon->titleId();
                Color  base    = m_theme.panelBase;
                Color  border  = m_theme.panelBorder;

                m_userSelect->show(
                    [this, focRect, tex, cr, base, border, tid](AccountUid uid) {
                        m_audio.playSfx(Sfx::LaunchGame);
                        m_launchAnim->start(focRect, tex, cr, base, border, tid, uid);
                    });
            }
        }
    }

    // --- Touch input ---
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
                // Second tap on already-focused icon → confirm (open user select)
                m_audio.playSfx(Sfx::Activate);
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
                        Rect   focRect = foc->getFocusRect();
                        const Texture* tex = icon->texture();
                        float  cr      = icon->cornerRadius();
                        uint64_t tid   = icon->titleId();
                        Color  base    = m_theme.panelBase;
                        Color  border  = m_theme.panelBorder;

                        m_userSelect->show(
                            [this, focRect, tex, cr, base, border, tid](AccountUid uid) {
                                m_audio.playSfx(Sfx::LaunchGame);
                                m_launchAnim->start(focRect, tex, cr, base, border, tid, uid);
                            });
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

void WiiUMenuApp::shutdown() {
    accountExit();
    nxtcExit();
    m_audio.shutdown();
    delete m_renderer;
    m_renderer = nullptr;
    m_gpu.shutdown();
}

} // namespace ui
