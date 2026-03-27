#include <nxui/widgets/GlassWidget.hpp>

namespace nxui {

Rect GlassWidget::glassContentRect() const {
    return {
        m_rect.x + m_padding.left,
        m_rect.y + m_padding.top,
        m_rect.width  - m_padding.left - m_padding.right,
        m_rect.height - m_padding.top  - m_padding.bottom
    };
}

void GlassWidget::sizeToFit() {
    Vec2 cs = computeContentSize();
    m_rect.width  = cs.x + m_padding.left + m_padding.right;
    m_rect.height = cs.y + m_padding.top  + m_padding.bottom;
}

void GlassWidget::onRender(Renderer& ren) {
    GlassPanel::onRender(ren);   // draw glass background
    onContentRender(ren);        // draw subclass content
}

void GlassWidget::onUpdate(float dt) {
    onContentUpdate(dt);
}

} // namespace nxui
