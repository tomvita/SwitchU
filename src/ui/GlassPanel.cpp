#include "GlassPanel.hpp"
#include "../core/Renderer.hpp"

namespace ui {

GlassPanel::GlassPanel() {}

void GlassPanel::onRender(Renderer& ren) {
    if (m_scale < 0.01f) return;

    Rect r = m_rect;
    if (m_scale < 1.f) {
        float w = r.width  * m_scale;
        float h = r.height * m_scale;
        r.x += (r.width  - w) * 0.5f;
        r.y += (r.height - h) * 0.5f;
        r.width  = w;
        r.height = h;
    }

    float op = m_opacity * m_panelOpacity;

    // Layer 1: main body (translucent)
    ren.drawRoundedRect(r, m_base.withAlpha(m_base.a * op), m_radius);

    // Layer 2: subtle inner glow (fake frosted depth)
    Rect inner = r.shrunk(1.f);
    ren.drawRoundedRect(inner, m_base.withAlpha(m_base.a * 0.25f * op), m_radius - 1.f);

    // // Layer 3: top-half highlight (glass shine)
    // Rect shine = {r.x, r.y, r.width, r.height * 0.40f};
    // ren.drawRoundedRect(shine, m_highlight.withAlpha(m_highlight.a * op), m_radius);

    // Layer 4: thin bright border
    if (m_borderW > 0.f)
        ren.drawRoundedRectOutline(r, m_border.withAlpha(m_border.a * op),
                                   m_radius, m_borderW);
}

} // namespace ui
