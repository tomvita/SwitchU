#pragma once
#include "../ui/Widget.hpp"
#include "../core/Animation.hpp"

namespace ui {

class SelectionCursor : public Widget {
public:
    SelectionCursor();

    void moveTo(const Rect& target, float duration = 0.2f);
    void moveTo(const Rect& target, float cornerRadius, float duration);
    void setColor(const Color& c) { m_color = c; }
    void setCornerRadius(float r)  { m_cornerRadius.setImmediate(r); }
    void setBorderWidth(float w)   { m_borderWidth = w; }

protected:
    void onUpdate(float dt) override;
    void onRender(Renderer& ren) override;

private:
    AnimatedFloat m_x, m_y, m_w, m_h;
    AnimatedFloat m_cornerRadius;
    Color m_color {0.f, 0.75f, 1.f, 1.f};
    float m_borderWidth  = 3.f;
    float m_time = 0.f;
    float m_waveSpeed = 3.5f;
    bool  m_initialized = false;
};

} // namespace ui
