#pragma once
#include "Widget.hpp"
#include "../core/Types.hpp"

namespace ui {

class GlassPanel : public Widget {
public:
    GlassPanel();

    void setCornerRadius(float r)     { m_radius = r; }
    void setBaseColor(const Color& c) { m_base = c; }
    void setBorderColor(const Color& c) { m_border = c; }
    void setHighlightColor(const Color& c) { m_highlight = c; }
    void setBorderWidth(float w)      { m_borderW = w; }
    void setScale(float s)            { m_scale = s; }
    void setPanelOpacity(float o)     { m_panelOpacity = o; }
    float scale() const               { return m_scale; }
    float panelOpacity() const        { return m_panelOpacity; }
    float cornerRadius() const        { return m_radius; }

protected:
    void onRender(Renderer& ren) override;

    float m_radius       = 24.f;
    float m_borderW      = 1.f;
    float m_scale        = 1.f;
    float m_panelOpacity = 1.f;
    Color m_base      {0.14f, 0.14f, 0.22f, 0.40f};
    Color m_border    {0.50f, 0.50f, 0.65f, 0.22f};
    Color m_highlight {1.f, 1.f, 1.f, 0.05f};
};

} // namespace ui
