#pragma once
#include "../ui/GlassWidget.hpp"
#include "../core/Font.hpp"
#include "../core/Types.hpp"

namespace ui {

class BatteryWidget : public GlassWidget {
public:
    BatteryWidget() = default;
    void setFont(Font* f) { m_font = f; }
    void setTextColor(const Color& c) { m_textColor = c; }

protected:
    void onContentUpdate(float dt) override;
    void onContentRender(Renderer& ren) override;
    Vec2 computeContentSize() const override;

private:
    Font* m_font = nullptr;
    float m_level   = 1.f;
    bool  m_charging = false;
    float m_timer    = 0.f;
    Color m_textColor {1.f, 1.f, 1.f, 1.f};
};

} // namespace ui
