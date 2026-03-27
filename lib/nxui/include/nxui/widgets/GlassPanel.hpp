#pragma once
#include <nxui/widgets/Box.hpp>
#include <nxui/core/Types.hpp>

namespace nxui {

class GlassPanel : public Box {
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

    // Blur: real Gaussian blur of the background behind this panel
    void setBlurEnabled(bool b)       { m_blurEnabled = b; }
    void setBlurRadius(float r)       { m_blurRadius = r; }
    void setBlurPasses(int p)         { m_blurPasses = p; }
    bool blurEnabled() const          { return m_blurEnabled; }

    // Opaque backing: draws a fully-opaque rounded rect BEFORE all glass
    // layers so that anything underneath (e.g. a screen dimmer) is blocked.
    void setBackingEnabled(bool b)       { m_backingEnabled = b; }
    void setBackingColor(const Color& c) { m_backingColor = c; }
    bool backingEnabled() const          { return m_backingEnabled; }

protected:
    void onRender(Renderer& ren) override;

    float m_radius       = 24.f;
    float m_borderW      = 1.f;
    float m_scale        = 1.f;
    float m_panelOpacity = 1.f;
    Color m_base      {0.14f, 0.14f, 0.22f, 0.40f};
    Color m_border    {0.50f, 0.50f, 0.65f, 0.22f};
    Color m_highlight {1.f, 1.f, 1.f, 0.05f};

    // Blur settings
    bool  m_blurEnabled  = false;
    float m_blurRadius   = 2.0f;
    int   m_blurPasses   = 3;

    // Opaque backing
    bool  m_backingEnabled = false;
    Color m_backingColor   {0.f, 0.f, 0.f, 1.f};
};

} // namespace nxui
