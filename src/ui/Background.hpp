#pragma once
#include "Widget.hpp"
#include "../core/Types.hpp"

namespace ui {

class Renderer;

// Abstract base for all background styles
class Background : public Widget {
public:
    virtual ~Background() = default;

    // Theme-driven colours
    void setAccentColor(const Color& c)  { m_accent = c; }
    void setSecondaryColor(const Color& c) { m_secondary = c; }
    void setShapeColor(const Color& c)   { m_shapeColor = c; }

    // Regenerate particles / shapes
    virtual void regenerate(int count = 50) = 0;

protected:
    Color m_accent    {0.08f, 0.06f, 0.16f, 1.f};
    Color m_secondary {0.04f, 0.04f, 0.08f, 1.f};
    Color m_shapeColor{0.22f, 0.18f, 0.38f, 0.10f};
};

} // namespace ui
