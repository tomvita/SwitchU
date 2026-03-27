#pragma once
#include <nxui/widgets/GlassWidget.hpp>
#include <nxui/core/Font.hpp>
#include <nxui/core/Types.hpp>
#include <string>


class TitlePillWidget : public nxui::GlassWidget {
public:
    TitlePillWidget() = default;

    void setFont(nxui::Font* f)              { m_font = f; }
    void setTextColor(const nxui::Color& c)  { m_textColor = c; }

    void setText(const std::string& text, float screenWidth = 1280.f);

protected:
    void onContentRender(nxui::Renderer& ren) override;
    nxui::Vec2 computeContentSize() const override;

private:
    nxui::Font*       m_font = nullptr;
    std::string m_text;
    nxui::Color       m_textColor {1.f, 1.f, 1.f, 1.f};
};

