#pragma once
#include "../ui/GlassWidget.hpp"
#include "../ui/Focusable.hpp"
#include "../core/Texture.hpp"
#include "../core/Animation.hpp"
#include <string>

namespace ui {

class Renderer;

class GlossyIcon : public GlassWidget, public IFocusable {
public:
    GlossyIcon();

    void setTitle(const std::string& t) { m_title = t; }
    const std::string& title() const    { return m_title; }

    void setTexture(Texture* tex) { m_tex = tex; }
    Texture* texture() const      { return m_tex; }

    void setTitleId(uint64_t id)  { m_titleId = id; }
    uint64_t titleId() const      { return m_titleId; }

    // Appear animation
    void startAppear(float delay);

    // IFocusable
    Rect getFocusRect() const override { return m_rect; }
    std::string getFocusId() const override { return m_title; }
    void onFocusGained() override { m_focused = true; }
    void onFocusLost()   override { m_focused = false; }

protected:
    void onRender(Renderer& ren) override;
    void onContentUpdate(float dt) override;
    void onContentRender(Renderer& ren) override;

private:
    std::string m_title;
    Texture*    m_tex = nullptr;
    uint64_t    m_titleId = 0;
    bool        m_focused = false;

    // Appear animation state
    AnimatedFloat m_animScale;
    AnimatedFloat m_appearOpacity;
    float         m_appearDelay = 0.f;
    float         m_appearTimer = 0.f;
    bool          m_appearing   = false;
};

} // namespace ui
