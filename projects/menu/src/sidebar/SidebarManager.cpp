#include "SidebarManager.hpp"
#include "core/DebugLog.hpp"
#include <nxui/core/I18n.hpp>


void SidebarManager::build(nxui::GpuDevice& gpu, nxui::Renderer& ren,
                           const std::string& assetsBase,
                           const Actions& actions) {
    std::string iconsBase = assetsBase + "/icons";
    static const char* iconFiles[] = {
        "album.png",
        "eshop.png",
        "controller.png",
        "power.png",
    };
    m_icons.clear();
    m_icons.resize(4);
    for (int i = 0; i < 4; ++i)
        m_icons[i].loadFromFile(gpu, ren, iconsBase + "/" + iconFiles[i]);

    constexpr float btnSize = 70.f;
    constexpr float gap     = 16.f;
    constexpr float marginX = 14.f;

    float leftX  = marginX;
    float rightX = 1280.f - marginX - btnSize;
    float totalH = 3 * btnSize + 2.f * gap;
    float startY = 360.f - totalH * 0.5f;

    m_leftButtons.clear();
    m_rightButtons.clear();
    m_settingsButton = nullptr;
    m_albumButton    = nullptr;
    m_anims.clear();

    auto makeBtn = [](nxui::Texture* tex, const std::string& labelKey,
                      const std::string& fallback, std::function<void()> action) {
        auto btn = std::make_shared<AppletButton>();
        btn->setIcon(tex);
        btn->setLabelKey(labelKey, fallback);
        btn->setOnActivate(std::move(action));
        btn->setFocusable(true);
        return btn;
    };

    {
        auto album = makeBtn(&m_icons[0], "sidebar.album", "Album", actions.onAlbum);
        m_albumButton = album.get();
        album->setIconCircular(true);
        album->setRect({leftX, startY + 0.f * (btnSize + gap), btnSize, btnSize});
        m_leftButtons.push_back(std::move(album));

        auto eshop = makeBtn(&m_icons[1], "sidebar.eshop", "eShop", actions.onEShop);
        eshop->setRect({leftX, startY + 1.f * (btnSize + gap), btnSize, btnSize});
        m_leftButtons.push_back(std::move(eshop));

        auto settings = makeBtn(&m_icons[1], "sidebar.settings", "Settings", actions.onSettings);
        m_settingsButton = settings.get();
        settings->setRect({leftX, startY + 2.f * (btnSize + gap), btnSize, btnSize});
        m_leftButtons.push_back(std::move(settings));
    }

    {
        auto ctrl = makeBtn(&m_icons[2], "sidebar.controllers", "Controllers", actions.onControllers);
        ctrl->setRect({rightX, startY + 0.f * (btnSize + gap), btnSize, btnSize});
        m_rightButtons.push_back(std::move(ctrl));

        auto sleep = makeBtn(&m_icons[3], "sidebar.sleep", "Sleep", actions.onSleep);
        sleep->setRect({rightX, startY + 1.f * (btnSize + gap), btnSize, btnSize});
        m_rightButtons.push_back(std::move(sleep));

        auto miiverse = makeBtn(&m_icons[1], "sidebar.miiverse", "Miiverse", actions.onMiiverse);
        miiverse->setRect({rightX, startY + 2.f * (btnSize + gap), btnSize, btnSize});
        m_rightButtons.push_back(std::move(miiverse));
    }

    static const struct { int iconIdx; const char* webpFile; } animDefs[] = {
        { 0, "album.webp"      },
        { 1, "eshop.webp"      },
        { 2, "controller.webp" },
        { 3, "power.webp"      },
    };

    auto buttonForIcon = [&](int iconIdx) -> AppletButton* {
        switch (iconIdx) {
            case 0: return m_leftButtons.size()  > 0 ? m_leftButtons[0].get()  : nullptr;
            case 1: return m_leftButtons.size()  > 1 ? m_leftButtons[1].get()  : nullptr;
            case 2: return m_rightButtons.size() > 0 ? m_rightButtons[0].get() : nullptr;
            case 3: return m_rightButtons.size() > 1 ? m_rightButtons[1].get() : nullptr;
            default: return nullptr;
        }
    };

    for (const auto& def : animDefs) {
        AppletButton* btn = buttonForIcon(def.iconIdx);
        if (!btn) continue;
        tryLoadAnimation(gpu, ren, iconsBase + "/" + def.webpFile,
                         btn, &m_icons[def.iconIdx]);
    }
}

void SidebarManager::tryLoadAnimation(nxui::GpuDevice& gpu, nxui::Renderer& ren,
                                      const std::string& webpPath,
                                      AppletButton* button,
                                      nxui::Texture* staticIcon) {
    AnimEntry entry;
    entry.button    = button;
    entry.staticTex = staticIcon;
    if (entry.anim.load(gpu, ren, webpPath)) {
        m_anims.push_back(std::move(entry));
    }
}


void SidebarManager::update(float dt, nxui::Widget* focusedWidget) {
    for (auto& e : m_anims) {
        if (!e.button) continue;
        bool focused = (focusedWidget == e.button);
        e.anim.update(dt, focused);

        if (focused && e.anim.hasFrames()) {
            e.button->setIcon(e.anim.currentFrame());
        } else {
            e.button->setIcon(e.staticTex);
        }
    }
}


void SidebarManager::applyTheme(const nxui::Theme& theme) {
    auto apply = [&](std::shared_ptr<AppletButton>& btn) {
        btn->setBaseColor(theme.panelBase);
        btn->setBorderColor(theme.panelBorder);
        btn->setHighlightColor(theme.panelHighlight);
    };
    for (auto& btn : m_leftButtons)  apply(btn);
    for (auto& btn : m_rightButtons) apply(btn);
}
