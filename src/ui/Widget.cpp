// ui/Widget.cpp
#include "Widget.hpp"
#include "../core/Renderer.hpp"
#include <algorithm>

namespace ui {

void Widget::addChild(std::shared_ptr<Widget> child) {
    m_children.push_back(std::move(child));
}

void Widget::removeChild(Widget* child) {
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [child](const auto& c){ return c.get() == child; }),
        m_children.end());
}

void Widget::clearChildren() { m_children.clear(); }

void Widget::update(float dt) {
    if (!m_visible) return;
    onUpdate(dt);
    for (auto& c : m_children) c->update(dt);
}

void Widget::render(Renderer& ren) {
    if (!m_visible || m_opacity <= 0.f) return;
    onRender(ren);
    for (auto& c : m_children) c->render(ren);
}

} // namespace ui
