#include "BatteryWidget.hpp"
#include "../core/Renderer.hpp"
#include <switch.h>
#include <cstdio>

namespace ui {

void BatteryWidget::onContentUpdate(float dt) {
    m_timer += dt;
    if (m_timer < 5.f && m_level > 0.f) return;
    m_timer = 0.f;

    u32 charge = 100;
    psmGetBatteryChargePercentage(&charge);
    m_level = charge / 100.f;

    PsmChargerType ct = PsmChargerType_Unconnected;
    psmGetChargerType(&ct);
    m_charging = (ct != PsmChargerType_Unconnected);
}

void BatteryWidget::onContentRender(Renderer& ren) {
    Rect cr = contentRect();
    float bw = 36.f, bh = 18.f;
    float bx = cr.x + (cr.width - bw - 4.f) * 0.5f;
    float by = cr.y + 2.f;
    float op = m_opacity;

    ren.drawRectOutline({bx, by, bw, bh}, m_textColor.withAlpha(0.7f * op), 1.5f);
    ren.drawRect({bx + bw, by + bh * 0.25f, 4, bh * 0.5f}, m_textColor.withAlpha(0.7f * op));

    Color fill = m_level > 0.2f ? Color(0.3f, 0.9f, 0.3f, op) : Color(0.9f, 0.2f, 0.2f, op);
    float innerW = (bw - 4) * m_level;
    ren.drawRect({bx + 2, by + 2, innerW, bh - 4}, fill);

    if (m_font) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d%%", (int)(m_level * 100));
        Vec2 sz = m_font->measure(buf);
        float tx = cr.x + (cr.width - sz.x * 0.75f) * 0.5f;
        float ty = by + bh + 4.f;
        ren.drawText(buf, {tx, ty}, m_font, m_textColor.withAlpha(op), 0.75f);
    }
}

Vec2 BatteryWidget::computeContentSize() const {
    float bw = 36.f, bh = 18.f;
    float textH = m_font ? m_font->measure("100%").y * 0.75f : 14.f;
    return {bw + 4.f, bh + 6.f + textH};
}

} // namespace ui
