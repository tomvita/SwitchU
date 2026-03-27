#include "TitlePillWidget.hpp"
#include <nxui/core/Renderer.hpp>


void TitlePillWidget::setText(const std::string& text, float screenWidth) {
    m_text = text;
    sizeToFit();
    setCornerRadius(m_rect.height * 0.5f);
    m_rect.x = (screenWidth - m_rect.width) * 0.5f;
}

void TitlePillWidget::onContentRender(nxui::Renderer& ren) {
    if (!m_font || m_text.empty()) return;

    nxui::Rect cr = contentRect();
    nxui::Vec2 textSz = m_font->measure(m_text);
    float tx = cr.x + (cr.width  - textSz.x) * 0.5f;
    float ty = cr.y + (cr.height - textSz.y) * 0.5f;
    ren.drawText(m_text, {tx, ty}, m_font, m_textColor.withAlpha(m_opacity), 1.f);
}

nxui::Vec2 TitlePillWidget::computeContentSize() const {
    if (!m_font || m_text.empty()) return {0.f, 0.f};
    return m_font->measure(m_text);
}

