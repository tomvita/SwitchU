#include "DateTimeWidget.hpp"
#include "../core/Renderer.hpp"
#include <ctime>
#include <cstdio>

namespace ui {

void DateTimeWidget::onContentUpdate(float dt) {
    m_timer += dt;
    if (m_timer < 1.f && !m_timeStr.empty()) return;
    m_timer = 0.f;

    std::time_t t = std::time(nullptr);
    std::tm* tm   = std::localtime(&t);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", tm->tm_hour, tm->tm_min);
    m_timeStr = buf;
    std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d",
                  tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    m_dateStr = buf;
}

void DateTimeWidget::onContentRender(Renderer& ren) {
    if (!m_font) return;

    Rect cr = contentRect();

    Vec2 timeSz = m_font->measure(m_timeStr);
    float tx = cr.x + (cr.width - timeSz.x) * 0.5f;
    float ty = cr.y;
    ren.drawText(m_timeStr, {tx, ty}, m_font, m_textColor.withAlpha(m_opacity), 1.f);

    Font* sf = m_smallFont ? m_smallFont : m_font;
    Vec2 dateSz = sf->measure(m_dateStr);
    float dx = cr.x + (cr.width - dateSz.x * 0.7f) * 0.5f;
    float dy = ty + timeSz.y + 2.f;
    ren.drawText(m_dateStr, {dx, dy}, sf, m_secondaryColor.withAlpha(m_opacity), 0.7f);
}

Vec2 DateTimeWidget::computeContentSize() const {
    if (!m_font) return {130.f, 46.f};
    Vec2 timeSz = m_font->measure("00:00");
    Font* sf = m_smallFont ? m_smallFont : m_font;
    Vec2 dateSz = sf->measure("00/00/0000");
    float w = std::max(timeSz.x, dateSz.x * 0.7f);
    float h = timeSz.y + 2.f + dateSz.y * 0.7f;
    return {w, h};
}

} // namespace ui
