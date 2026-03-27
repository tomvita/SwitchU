#include <nxui/widgets/GlassPanel.hpp>
#include <nxui/core/Renderer.hpp>

namespace nxui {

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

    // Opaque backing: fully blocks anything drawn underneath
    if (m_backingEnabled && op > 0.01f) {
        ren.drawRoundedRect(r, m_backingColor.withAlpha(op), m_radius);
    }

    // Real frosted glass: capture + blur + composite behind panel
    if (m_blurEnabled && op > 0.01f) {
        // Capture current framebuffer to offscreen target 0
        ren.captureToOffscreen();
        // Apply multi-pass Gaussian blur
        ren.applyBlur(m_blurRadius, m_blurPasses);
        // Draw the blurred result clipped to this panel's rounded rect area
        ren.pushClipRect(r);
        Rect fullScreen = {0, 0, (float)ren.width(), (float)ren.height()};
        ren.drawOffscreen(0, fullScreen, Color::white().withAlpha(op));
        ren.popClipRect();
    }

    // Layer 1: translucent tinted body
    ren.drawRoundedRect(r, m_base.withAlpha(m_base.a * op), m_radius);

    // Layer 2: iOS-style top-edge highlight (white shine clipped to upper portion)
    if (m_highlight.a > 0.01f && op > 0.01f) {
        Rect topClip = { r.x, r.y, r.width, r.height * 0.45f };
        ren.pushClipRect(topClip);
        ren.drawRoundedRectOutline(r.shrunk(0.5f),
                                   m_highlight.withAlpha(m_highlight.a * op),
                                   m_radius - 0.5f, 1.f);
        ren.popClipRect();
    }

    // Layer 3: subtle outer border
    if (m_borderW > 0.f)
        ren.drawRoundedRectOutline(r, m_border.withAlpha(m_border.a * op),
                                   m_radius, m_borderW);
}

} // namespace nxui
