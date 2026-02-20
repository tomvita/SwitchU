#pragma once
#include "../ui/Widget.hpp"
#include "../ui/GlassPanel.hpp"
#include "../core/Animation.hpp"
#include "../core/Texture.hpp"
#include <switch/services/acc.h>

namespace ui {

class LaunchAnimation : public Widget {
public:
    LaunchAnimation() = default;

    // Start: zoom the icon from its current position to screen center, then launch
    void start(const Rect& from, const Texture* tex, float cornerRadius,
               const Color& panelColor, const Color& borderColor,
               uint64_t titleId, AccountUid uid, VoidCallback onDone = {});

    bool isPlaying() const { return m_playing; }

protected:
    void onUpdate(float dt) override;
    void onRender(Renderer& ren) override;

private:
    bool  m_playing  = false;
    float m_timer    = 0.f;

    // Duration constants for each phase
    static constexpr float kZoomDur  = 0.45f;
    static constexpr float kHoldDur  = 0.35f;
    static constexpr float kFadeDur  = 0.25f;
    static constexpr float kTotalDur = kZoomDur + kHoldDur + kFadeDur;

    Rect     m_from;
    Rect     m_target;
    float    m_cornerRadius = 24.f;
    const Texture* m_tex = nullptr;
    Color   m_panelColor;
    Color   m_borderColor;
    uint64_t m_titleId = 0;
    AccountUid m_uid = {};
    VoidCallback m_onDone;
    bool    m_launched = false;
};

} // namespace ui
