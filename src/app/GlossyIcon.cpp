#include "GlossyIcon.hpp"
#include "../core/Renderer.hpp"
#include "../core/Font.hpp"
#include <cmath>

namespace ui {

GlossyIcon::GlossyIcon() {
    m_animScale.setImmediate(0.f);
    m_appearOpacity.setImmediate(0.f);
    setCornerRadius(16.f);
    setPadding(8.f);
}

void GlossyIcon::startAppear(float delay) {
    m_appearDelay = delay;
    m_appearTimer = 0.f;
    m_appearing   = true;
    m_animScale.setImmediate(0.f);
    m_appearOpacity.setImmediate(0.f);
}

void GlossyIcon::onContentUpdate(float dt) {
    if (m_appearing) {
        m_appearTimer += dt;
        if (m_appearTimer >= m_appearDelay) {
            m_appearing = false;
            m_animScale.set(1.f, 0.4f, Easing::outExpo);
            m_appearOpacity.set(1.f, 0.3f, Easing::outExpo);
        }
    }
    if (m_suspended) {
        m_suspendPulse += dt * 2.2f; // ~2.2 Hz oscillation
    }
}

void GlossyIcon::onRender(Renderer& ren) {
    float s = m_animScale.value();
    float a = m_appearOpacity.value();
    if (s < 0.01f || a < 0.01f) return;

    // Set GlassPanel scale for the glass background
    setScale(s);

    // Temporarily combine appear opacity with widget opacity
    float savedOp = m_opacity;
    m_opacity = a * savedOp;

    // Draw glass background + content via template method
    GlassWidget::onRender(ren);

    m_opacity = savedOp;

    // --- Suspended indicator: pulsing green border + play badge ---
    if (m_suspended && s > 0.5f) {
        float pulse = 0.5f + 0.5f * std::sin(m_suspendPulse);
        float glowAlpha = 0.35f + 0.25f * pulse;

        // Compute scaled rect
        Rect r = m_rect;
        if (s < 1.f) {
            float w = r.width  * s;
            float h = r.height * s;
            r.x += (r.width  - w) * 0.5f;
            r.y += (r.height - h) * 0.5f;
            r.width  = w;
            r.height = h;
        }
        float rad = cornerRadius();

        // Green glow border (2 passes for thickness)
        Color glow(0.18f, 0.85f, 0.45f, glowAlpha * a);
        ren.drawRoundedRectOutline(r.expanded(2.f), glow, rad + 2.f, 2.5f);

        // Small play triangle badge (bottom-right corner)
        float badgeSize = 26.f * s;
        float badgeX = r.x + r.width  - badgeSize - 4.f * s;
        float badgeY = r.y + r.height - badgeSize - 4.f * s;

        // Badge background circle
        Vec2 badgeCenter = { badgeX + badgeSize * 0.5f, badgeY + badgeSize * 0.5f };
        ren.drawCircle(badgeCenter, badgeSize * 0.5f, Color(0.1f, 0.1f, 0.1f, 0.85f * a), 16);

        // Play triangle (pointing right)
        float triH = badgeSize * 0.45f;
        float triW = triH * 0.85f;
        Vec2 p1 = { badgeCenter.x - triW * 0.35f, badgeCenter.y - triH * 0.5f };
        Vec2 p2 = { badgeCenter.x - triW * 0.35f, badgeCenter.y + triH * 0.5f };
        Vec2 p3 = { badgeCenter.x + triW * 0.65f, badgeCenter.y };
        ren.drawTriangle(p1, p2, p3, Color(0.18f, 0.85f, 0.45f, 0.95f * a));
    }
}

void GlossyIcon::onContentRender(Renderer& ren) {
    if (!m_tex || !m_tex->valid()) return;

    float s = scale();
    float rad = cornerRadius();

    // Compute the scaled rect (same logic as GlassPanel)
    Rect r = m_rect;
    if (s < 1.f) {
        float w = r.width  * s;
        float h = r.height * s;
        r.x += (r.width  - w) * 0.5f;
        r.y += (r.height - h) * 0.5f;
        r.width  = w;
        r.height = h;
    }

    float inset = 8.f * s;
    Rect texRect = r.shrunk(inset);
    ren.drawTextureRounded(m_tex, texRect, rad - 3.f,
                           Color::white().withAlpha(m_opacity));
}

} // namespace ui
