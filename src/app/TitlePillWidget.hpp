#pragma once
#include "../ui/GlassWidget.hpp"
#include "../core/Font.hpp"
#include "../core/Types.hpp"
#include <string>

namespace ui {

/**
 * TitlePillWidget: a GlassWidget shaped as a capsule that displays
 * centred text.  Calling setText() automatically resizes the pill.
 */
class TitlePillWidget : public GlassWidget {
public:
    TitlePillWidget() = default;

    void setFont(Font* f)              { m_font = f; }
    void setTextColor(const Color& c)  { m_textColor = c; }

    /// Change the label text and auto-resize (centred on screen).
    void setText(const std::string& text, float screenWidth = 1280.f);

protected:
    void onContentRender(Renderer& ren) override;
    Vec2 computeContentSize() const override;

private:
    Font*       m_font = nullptr;
    std::string m_text;
    Color       m_textColor {1.f, 1.f, 1.f, 1.f};
};

} // namespace ui
