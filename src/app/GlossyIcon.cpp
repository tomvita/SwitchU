#include "GlossyIcon.hpp"
#include "../core/Renderer.hpp"
#include "../core/Font.hpp"

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
