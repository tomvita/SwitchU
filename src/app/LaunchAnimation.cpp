#include "LaunchAnimation.hpp"
#include "../core/Renderer.hpp"
#include <cmath>

namespace ui {

void LaunchAnimation::start(const Rect& from, const Texture* tex, float cornerRadius,
                            const Color& panelColor, const Color& borderColor,
                            uint64_t titleId, AccountUid uid,
                            LaunchCallback onLaunch, VoidCallback onDone)
{
    m_from         = from;
    m_tex          = tex;
    m_cornerRadius = cornerRadius;
    m_panelColor   = panelColor;
    m_borderColor  = borderColor;
    m_titleId      = titleId;
    m_uid          = uid;
    m_onLaunch     = std::move(onLaunch);
    m_onDone       = std::move(onDone);
    m_timer        = 0.f;
    m_playing      = true;
    m_launched     = false;

    float side = std::min(from.width, from.height) * 1.8f;
    m_target = {(1280 - side) * 0.5f, (720 - side) * 0.5f, side, side};
}

void LaunchAnimation::onUpdate(float dt) {
    if (!m_playing) return;
    m_timer += dt;

    if (m_timer >= kTotalDur) {
        m_playing = false;
        // Launch the game once the full animation has finished
        if (!m_launched) {
            m_launched = true;
            if (m_titleId != 0 && m_onLaunch) {
                m_onLaunch(m_titleId, m_uid);
            }
        }
        if (m_onDone) m_onDone();
    }
}

void LaunchAnimation::onRender(Renderer& ren) {
    if (!m_playing) return;

    float t = m_timer;

    float zoomT = std::min(t / kZoomDur, 1.f);
    float ez = Easing::outCubic(zoomT);

    float x = m_from.x + (m_target.x - m_from.x) * ez;
    float y = m_from.y + (m_target.y - m_from.y) * ez;
    float w = m_from.width  + (m_target.width  - m_from.width)  * ez;
    float h = m_from.height + (m_target.height - m_from.height) * ez;
    float cr = m_cornerRadius * (1.f + 0.3f * ez);

    float pulseScale = 1.f;
    if (t > kZoomDur && t < kZoomDur + kHoldDur) {
        float pt = (t - kZoomDur) / kHoldDur;
        pulseScale = 1.f + 0.04f * std::sin(pt * 3.14159f * 2.f);
    }

    float fadeAlpha = 1.f;
    if (t > kZoomDur + kHoldDur) {
        float ft = (t - kZoomDur - kHoldDur) / kFadeDur;
        fadeAlpha = 1.f - Easing::outCubic(std::min(ft, 1.f));
    }

    // Apply pulse
    float pw = w * pulseScale;
    float ph = h * pulseScale;
    float px = x + (w - pw) * 0.5f;
    float py = y + (h - ph) * 0.5f;
    Rect animRect = {px, py, pw, ph};

    // Dim background overlay
    float dimAlpha = 0.5f * ez * fadeAlpha;
    ren.drawRect({0, 0, 1280, 720}, Color(0, 0, 0, dimAlpha));

    // Glass panel body
    Color body = m_panelColor.withAlpha(m_panelColor.a * fadeAlpha);
    ren.drawRoundedRect(animRect, body, cr);

    // Glass highlight
    Rect shine = {animRect.x, animRect.y, animRect.width, animRect.height * 0.4f};
    ren.drawRoundedRect(shine, Color(1, 1, 1, 0.06f * fadeAlpha), cr);

    // Glass border
    Color border = m_borderColor.withAlpha(m_borderColor.a * fadeAlpha);
    ren.drawRoundedRectOutline(animRect, border, cr, 1.f);

    // Icon texture inside
    if (m_tex && m_tex->valid()) {
        Rect texRect = animRect.shrunk(8.f);
        ren.drawTextureRounded(m_tex, texRect, cr - 4.f,
                               Color::white().withAlpha(fadeAlpha));
    }
}

} // namespace ui
