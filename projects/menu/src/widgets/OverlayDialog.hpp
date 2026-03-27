#pragma once
#include <nxui/widgets/GlassWidget.hpp>
#include <nxui/widgets/Label.hpp>
#include <nxui/widgets/Box.hpp>
#include <nxui/core/Animation.hpp>
#include <nxui/core/Font.hpp>
#include <nxui/core/Input.hpp>
#include <nxui/Theme.hpp>
#include "SelectionCursor.hpp"
#include <vector>
#include <string>
#include <functional>

class OverlayDialog : public nxui::GlassWidget {
public:
    struct ButtonDef {
        std::string label;
        std::function<void()> onPress;
        bool closeOnPress = true;
    };

    using CancelCallback = std::function<void()>;
    using VoidCb = std::function<void()>;

    OverlayDialog();

    void setFont(nxui::Font* f)         { m_font = f; }
    void setSmallFont(nxui::Font* f)    { m_smallFont = f; }
    void setTheme(const nxui::Theme* t) { m_theme = t; }

    void show(const std::string& title,
              const std::string& message,
              std::vector<ButtonDef> buttons,
              int initialSelected = 0,
              CancelCallback onCancel = {});

    void hide();

    bool isActive() const { return m_active || m_animatingOut; }

    void handleTouch(nxui::Input& input);

    void onNavigateSfx(VoidCb cb)  { m_navSfxCb = std::move(cb); }
    void onActivateSfx(VoidCb cb)  { m_activateSfxCb = std::move(cb); }
    void onCloseSfx(VoidCb cb)     { m_closeSfxCb = std::move(cb); }

    SelectionCursor& cursor() { return m_cursor; }

    void update(float dt) override;
    void render(nxui::Renderer& ren) override;

private:
    void buildWidgetTree();
    void animateButtonFocus(float duration, nxui::EasingFunc easing);
    void setupActions();
    void activateSelected();
    void cancel();
    void syncCursor();
    void syncChildOpacities();

    nxui::Rect panelRect() const;

    nxui::Font*        m_font      = nullptr;
    nxui::Font*        m_smallFont = nullptr;
    const nxui::Theme* m_theme     = nullptr;

    std::string              m_title;
    std::string              m_message;
    std::vector<ButtonDef>   m_buttons;
    int  m_selected     = 0;
    bool m_active       = false;
    bool m_animatingOut = false;

    std::shared_ptr<nxui::Label>                     m_titleLabel;
    std::shared_ptr<nxui::Label>                     m_messageLabel;
    std::shared_ptr<nxui::Box>                       m_buttonRow;
    std::vector<std::shared_ptr<nxui::GlassWidget>>  m_btnWidgets;
    SelectionCursor m_cursor;

    nxui::AnimatedFloat m_overlayAlpha;
    nxui::AnimatedFloat m_panelScale;
    nxui::AnimatedFloat m_contentReveal;
    std::vector<nxui::AnimatedFloat> m_buttonFocus;

    float m_panelH = 0.f;

    CancelCallback m_onCancel;
    VoidCb         m_navSfxCb;
    VoidCb         m_activateSfxCb;
    VoidCb         m_closeSfxCb;

    int  m_touchHitButton  = -1;
    bool m_touchOnSelected = false;

    static constexpr float kPanelW       = 560.f;
    static constexpr float kPanelPadX    = 32.f;
    static constexpr float kPanelPadY    = 28.f;
    static constexpr float kPanelRadius  = 24.f;
    static constexpr float kButtonH      = 48.f;
    static constexpr float kButtonRadius = 14.f;
    static constexpr float kButtonGap    = 12.f;
    static constexpr float kTitleMsgGap  = 10.f;
    static constexpr float kMsgBtnGap    = 20.f;
};
