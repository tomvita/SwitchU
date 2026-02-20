#pragma once
#include "../core/Types.hpp"
#include <vector>
#include <memory>
#include <string>

namespace ui {

class Renderer;

class Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;

    // Tree
    void addChild(std::shared_ptr<Widget> child);
    void removeChild(Widget* child);
    void clearChildren();
    const std::vector<std::shared_ptr<Widget>>& children() const { return m_children; }

    // Geometry
    void setPosition(float x, float y) { m_rect.x = x; m_rect.y = y; }
    void setSize(float w, float h)     { m_rect.width = w; m_rect.height = h; }
    void setRect(const Rect& r)        { m_rect = r; }
    const Rect& rect() const           { return m_rect; }

    // Visibility & opacity
    void  setVisible(bool v)   { m_visible = v; }
    bool  isVisible() const    { return m_visible; }
    void  setOpacity(float a)  { m_opacity = a; }
    float opacity() const      { return m_opacity; }

    // Tags / debug
    void setTag(const std::string& t) { m_tag = t; }
    const std::string& tag() const    { return m_tag; }

    // Lifecycle (override in subclasses)
    virtual void update(float dt);
    virtual void render(Renderer& ren);

protected:
    virtual void onUpdate(float dt) {}
    virtual void onRender(Renderer& ren) {}

    Rect  m_rect;
    float m_opacity = 1.f;
    bool  m_visible = true;
    std::string m_tag;
    std::vector<std::shared_ptr<Widget>> m_children;
};

} // namespace ui
