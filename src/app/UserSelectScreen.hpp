#pragma once
#include "../ui/GlassPanel.hpp"
#include "SelectionCursor.hpp"
#include "../core/Texture.hpp"
#include "../core/Font.hpp"
#include "../core/Types.hpp"
#include "../core/Animation.hpp"
#include <switch.h>
#include <vector>
#include <string>
#include <functional>

namespace ui {

class GpuDevice;
class Renderer;
class AudioManager;

/**
 * Full-screen user-selection overlay.
 * Shows all system accounts as circular avatars + name.
 * User picks one with D-pad/stick and confirms with A, cancels with B.
 */
class UserSelectScreen : public Widget {
public:
    using SelectCallback = std::function<void(AccountUid uid)>;
    using CancelCallback = std::function<void()>;

    UserSelectScreen() = default;

    /// Load accounts from the system.  Must be called after GPU is ready.
    bool loadUsers(GpuDevice& gpu, Renderer& ren);

    void setFont(Font* f)       { m_font = f; }
    void setSmallFont(Font* sf) { m_smallFont = sf; }
    void setAudio(AudioManager* a) { m_audio = a; }

    /// Show the overlay (animates in)
    void show(SelectCallback onSelect, CancelCallback onCancel = {});
    /// Hide (animates out, then calls done)
    void hide();

    bool isActive() const { return m_active; }

    /// Called by WiiUMenuApp each frame
    void handleInput(class Input& input);

    /// Access the glass panels for theming
    GlassPanel& panel() { return m_panel; }
    GlassPanel& titlePanel() { return m_titlePanel; }
    SelectionCursor& cursor() { return m_cursor; }

    void setTextColor(const Color& c)          { m_textPrimary = c; }
    void setSecondaryTextColor(const Color& c) { m_textSecondary = c; }

protected:
    void onUpdate(float dt) override;
    void onRender(Renderer& ren) override;

private:
    struct UserEntry {
        AccountUid    uid;
        std::string   nickname;
        Texture       icon;          // 256×256 JPEG → GPU texture
    };

    std::vector<UserEntry> m_users;
    int  m_selected = 0;
    bool m_active   = false;

    Font* m_font      = nullptr;
    Font* m_smallFont = nullptr;
    AudioManager* m_audio = nullptr;

    SelectCallback m_onSelect;
    CancelCallback m_onCancel;

    // Glass panels
    GlassPanel m_panel;
    GlassPanel m_titlePanel;
    SelectionCursor m_cursor;

    // Animation
    AnimatedFloat m_overlayAlpha;
    AnimatedFloat m_panelScale;
    bool m_animatingOut = false;

    // Theme colors (non-panel)
    Color m_textPrimary    {1.f, 1.f, 1.f, 1.f};
    Color m_textSecondary  {0.7f, 0.7f, 0.8f, 0.8f};

    // Cached avatar rects for touch hit-testing (updated each render)
    std::vector<Rect> m_avatarRects;

    // Touch state for two-tap interaction
    int m_touchHitAvatar = -1;    // avatar index under finger, or -1
    bool m_touchOnSelected = false; // finger landed on already-selected avatar
};

} // namespace ui
