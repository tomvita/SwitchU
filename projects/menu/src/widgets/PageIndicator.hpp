#pragma once
#include <nxui/widgets/GlassWidget.hpp>
#include <nxui/Theme.hpp>

class PageIndicator : public nxui::GlassWidget {
public:
    PageIndicator();

    void setTheme(const nxui::Theme* theme) { m_theme = theme; }
    void setPageCount(int total);
    void setCurrentPage(int page);

protected:
    void onContentRender(nxui::Renderer& ren) override;
    nxui::Vec2 computeContentSize() const override;

private:
    void updateGeometryFromContent();

    int m_total   = 1;
    int m_current = 0;
    float m_layoutWidth = 1280.f;
    bool m_geometryReady = false;
    const nxui::Theme* m_theme = nullptr;
};
