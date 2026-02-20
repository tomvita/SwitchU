#pragma once
#include "../ui/GlassWidget.hpp"
#include "../core/Font.hpp"
#include "../core/Types.hpp"
#include <string>

namespace ui {

class DateTimeWidget : public GlassWidget {
public:
    DateTimeWidget() = default;
    void setFont(Font* f) { m_font = f; }
    void setSmallFont(Font* sf) { m_smallFont = sf; }
    void setTextColor(const Color& c) { m_textColor = c; }
    void setSecondaryTextColor(const Color& c) { m_secondaryColor = c; }

protected:
    void onContentUpdate(float dt) override;
    void onContentRender(Renderer& ren) override;
    Vec2 computeContentSize() const override;

private:
    Font* m_font = nullptr;
    Font* m_smallFont = nullptr;
    float m_timer = 0.f;
    std::string m_timeStr;
    std::string m_dateStr;
    Color m_textColor      {1.f, 1.f, 1.f, 1.f};
    Color m_secondaryColor {0.7f, 0.7f, 0.8f, 0.8f};
};

} // namespace ui
