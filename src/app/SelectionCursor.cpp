#include "SelectionCursor.hpp"
#include "../core/Renderer.hpp"
#include <cmath>

namespace ui {

SelectionCursor::SelectionCursor() {
    m_x.setImmediate(0); m_y.setImmediate(0);
    m_w.setImmediate(0); m_h.setImmediate(0);
    m_cornerRadius.setImmediate(18.f);
}

void SelectionCursor::moveTo(const Rect& target, float duration) {
    if (!m_initialized) {
        m_x.setImmediate(target.x);
        m_y.setImmediate(target.y);
        m_w.setImmediate(target.width);
        m_h.setImmediate(target.height);
        m_initialized = true;
    } else {
        m_x.set(target.x, duration, Easing::outCubic);
        m_y.set(target.y, duration, Easing::outCubic);
        m_w.set(target.width, duration, Easing::outCubic);
        m_h.set(target.height, duration, Easing::outCubic);
    }
}

void SelectionCursor::moveTo(const Rect& target, float cornerRadius, float duration) {
    if (!m_initialized) {
        m_x.setImmediate(target.x);
        m_y.setImmediate(target.y);
        m_w.setImmediate(target.width);
        m_h.setImmediate(target.height);
        m_cornerRadius.setImmediate(cornerRadius);
        m_initialized = true;
    } else {
        m_x.set(target.x, duration, Easing::outCubic);
        m_y.set(target.y, duration, Easing::outCubic);
        m_w.set(target.width, duration, Easing::outCubic);
        m_h.set(target.height, duration, Easing::outCubic);
        m_cornerRadius.set(cornerRadius, duration, Easing::outCubic);
    }
}

void SelectionCursor::onUpdate(float dt) {
    m_time += dt;
}

void SelectionCursor::onRender(Renderer& ren) {
    if (!m_initialized || m_opacity <= 0.01f) return;

    float x = m_x.value(), y = m_y.value();
    float w = m_w.value(), h = m_h.value();
    if (w < 1.f || h < 1.f) return;

    Rect r = {x, y, w, h};
    float cr = m_cornerRadius.value();

    float wave = std::sin(m_time * m_waveSpeed) * 0.5f + 0.5f;

    Rect glowRect = r.expanded(6.f);
    Color glowC = m_color.withAlpha(0.10f + 0.05f * wave);
    glowC.a *= m_opacity;
    ren.drawRoundedRect(glowRect, glowC, cr + 8.f);

    Rect glow2 = r.expanded(3.f);
    Color glow2C = m_color.withAlpha(0.18f + 0.07f * wave);
    glow2C.a *= m_opacity;
    ren.drawRoundedRect(glow2, glow2C, cr + 4.f);

    Color mainC = m_color.withAlpha(m_opacity);
    ren.drawRoundedRectOutline(r, mainC, cr, m_borderWidth);

    Rect inner = r.shrunk(m_borderWidth * 0.5f);
    Color innerC = Color(0.3f, 0.85f, 1.f, 0.25f * m_opacity * (0.7f + 0.3f * wave));
    ren.drawRoundedRectOutline(inner, innerC, cr - 2.f, 1.5f);
}

} // namespace ui
