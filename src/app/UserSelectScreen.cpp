#include "UserSelectScreen.hpp"
#include "AudioManager.hpp"
#include "../core/Renderer.hpp"
#include "../core/GpuDevice.hpp"
#include "../core/Input.hpp"
#include <switch.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace ui {

// ────────────────────────────────────────────────────────────
// Load every system account: nickname + profile icon → GPU
// ────────────────────────────────────────────────────────────
bool UserSelectScreen::loadUsers(GpuDevice& gpu, Renderer& ren) {
    AccountUid uids[8] = {};
    s32 count = 0;
    Result rc = accountListAllUsers(uids, 8, &count);
    if (R_FAILED(rc) || count <= 0) return false;

    for (int i = 0; i < count; ++i) {
        AccountProfile profile;
        rc = accountGetProfile(&profile, uids[i]);
        if (R_FAILED(rc)) continue;

        // Nickname
        AccountProfileBase base;
        AccountUserData userData;
        rc = accountProfileGet(&profile, &userData, &base);
        if (R_FAILED(rc)) {
            accountProfileClose(&profile);
            continue;
        }

        UserEntry entry;
        entry.uid = uids[i];
        entry.nickname = base.nickname;

        // Profile image (JPEG)
        u32 imgSize = 0;
        u32 realSize = 0;
        rc = accountProfileGetImageSize(&profile, &imgSize);
        if (R_SUCCEEDED(rc) && imgSize > 0) {
            std::vector<uint8_t> imgBuf(imgSize);
            rc = accountProfileLoadImage(&profile, imgBuf.data(), imgSize, &realSize);
            if (R_SUCCEEDED(rc) && realSize > 0) {
                entry.icon.loadFromMemory(gpu, ren, imgBuf.data(), realSize);
            }
        }

        accountProfileClose(&profile);
        m_users.push_back(std::move(entry));
    }

    return !m_users.empty();
}

// ────────────────────────────────────────────────────────────
void UserSelectScreen::show(SelectCallback onSelect, CancelCallback onCancel) {
    // If no users were loaded, skip selection and pass a zeroed UID
    if (m_users.empty()) {
        if (onSelect) {
            AccountUid uid = {};
            onSelect(uid);
        }
        return;
    }

    m_onSelect = std::move(onSelect);
    m_onCancel = std::move(onCancel);
    m_active   = true;
    m_animatingOut = false;
    m_overlayAlpha.setImmediate(0.f);
    m_panelScale.setImmediate(0.85f);
    m_overlayAlpha.set(1.f, 0.2f, Easing::outExpo);
    m_panelScale.set(1.f, 0.25f, Easing::outExpo);
    if (m_audio) m_audio->playSfx(Sfx::ModalShow);
}

void UserSelectScreen::hide() {
    m_animatingOut = true;
    m_overlayAlpha.set(0.f, 0.15f, Easing::outExpo);
    m_panelScale.set(0.9f, 0.15f, Easing::outExpo);
}

// ────────────────────────────────────────────────────────────
void UserSelectScreen::handleInput(Input& input) {
    if (!m_active || m_animatingOut) return;

    int n = (int)m_users.size();
    if (n == 0) return;

    if (input.isDown(Button::DLeft) || input.isDown(Button::LStickL)) {
        m_selected = (m_selected - 1 + n) % n;
        if (m_audio) m_audio->playSfx(Sfx::Navigate);
    }
    if (input.isDown(Button::DRight) || input.isDown(Button::LStickR)) {
        m_selected = (m_selected + 1) % n;
        if (m_audio) m_audio->playSfx(Sfx::Navigate);
    }

    if (input.isDown(Button::A)) {
        if (m_audio) m_audio->playSfx(Sfx::Activate);
        AccountUid uid = m_users[m_selected].uid;
        auto cb = std::move(m_onSelect);
        hide();
        if (cb) cb(uid);
    }

    if (input.isDown(Button::B)) {
        if (m_audio) m_audio->playSfx(Sfx::ModalHide);
        auto cb = std::move(m_onCancel);
        hide();
        if (cb) cb();
    }

    // Touch: first tap selects avatar, second tap on selected confirms
    if (input.touchDown()) {
        float tx = input.touchX();
        float ty = input.touchY();
        m_touchHitAvatar = -1;
        m_touchOnSelected = false;
        for (int i = 0; i < (int)m_avatarRects.size(); ++i) {
            if (m_avatarRects[i].contains(tx, ty)) {
                m_touchHitAvatar = i;
                break;
            }
        }
        if (m_touchHitAvatar >= 0 && m_touchHitAvatar == m_selected) {
            m_touchOnSelected = true;
        }
    }

    if (input.touchUp()) {
        float dx = std::abs(input.touchDeltaX());
        float dy = std::abs(input.touchDeltaY());
        if (dx < 20.f && dy < 20.f) {
            if (m_touchHitAvatar >= 0 && m_touchHitAvatar < n) {
                if (m_touchOnSelected) {
                    // Second tap on already-selected → confirm
                    if (m_audio) m_audio->playSfx(Sfx::Activate);
                    AccountUid uid = m_users[m_selected].uid;
                    auto cb = std::move(m_onSelect);
                    hide();
                    if (cb) cb(uid);
                } else {
                    // First tap on a different avatar → just select it
                    m_selected = m_touchHitAvatar;
                    if (m_audio) m_audio->playSfx(Sfx::Navigate);
                }
            } else {
                // Tap outside avatars → cancel
                if (m_audio) m_audio->playSfx(Sfx::ModalHide);
                auto cb = std::move(m_onCancel);
                hide();
                if (cb) cb();
            }
        }
        m_touchHitAvatar = -1;
    }
}

// ────────────────────────────────────────────────────────────
void UserSelectScreen::onUpdate(float dt) {
    if (m_animatingOut && m_overlayAlpha.value() < 0.01f) {
        m_active = false;
        m_animatingOut = false;
    }

    // Sync panel scale animation
    m_panel.setScale(m_panelScale.value());
    m_panel.setOpacity(m_overlayAlpha.value());
    m_titlePanel.setScale(m_panelScale.value());
    m_titlePanel.setOpacity(m_overlayAlpha.value());
    m_cursor.setOpacity(m_overlayAlpha.value());
    m_cursor.update(dt);
}

void UserSelectScreen::onRender(Renderer& ren) {
    if (!m_active) return;

    float alpha = m_overlayAlpha.value();
    float sc    = m_panelScale.value();
    int n       = (int)m_users.size();
    if (n == 0 || alpha < 0.01f) return;

    // Dim background
    ren.drawRect({0, 0, 1280, 720}, Color(0, 0, 0, 0.55f * alpha));

    // Panel dimensions
    float avatarSize = 96.f;
    float gap        = 32.f;
    float itemW      = avatarSize;
    float totalW     = n * itemW + (n - 1) * gap;
    float panelPadX  = 48.f;
    float panelPadY  = 36.f;
    float nameH      = m_font ? m_font->measure("Ag").y : 20.f;
    float contentH   = avatarSize + 12.f + nameH;
    float panelW     = totalW + panelPadX * 2.f;
    float panelH     = contentH + panelPadY * 2.f;

    // Title text height
    float titleH = m_font ? m_font->measure("A").y * 1.1f : 28.f;
    float totalH = titleH + 16.f + panelH;

    float cx = 1280.f * 0.5f;
    float cy = 720.f  * 0.5f;

    float scaledTotalH = totalH * sc;

    // Title: "Who's playing?"
    if (m_font) {
        const std::string title = "Who's playing?";
        Vec2 titleSz = m_font->measure(title);
        float tPadX = 20.f;
        float tPadY = 8.f;
        float tPillW = titleSz.x * sc + tPadX * 2.f;
        float tPillH = titleSz.y * sc + tPadY * 2.f;
        float tPillX = cx - tPillW * 0.5f;
        float tPillY = cy - scaledTotalH * 0.5f - tPadY;
        m_titlePanel.setCornerRadius(tPillH * 0.5f);
        m_titlePanel.setRect({tPillX, tPillY, tPillW, tPillH});
        m_titlePanel.render(ren);

        float tx = cx - titleSz.x * 0.5f * sc;
        float ty = tPillY + tPadY;
        ren.drawText(title, {tx, ty}, m_font,
                     m_textPrimary.withAlpha(alpha), sc);
    }

    // Position the GlassPanel and let it render itself
    float panelX = cx - panelW * 0.5f;
    float panelY = cy - totalH * 0.5f + (titleH + 16.f);
    m_panel.setRect({panelX, panelY, panelW, panelH});
    m_panel.render(ren);

    // Compute the actual rendered rect (after scale) for content positioning
    float scaledPanelW = panelW * sc;
    float scaledPanelH = panelH * sc;
    float rpX = cx - scaledPanelW * 0.5f;
    float rpY = panelY + (panelH - scaledPanelH) * 0.5f;

    // Draw each user
    float startX = rpX + panelPadX * sc;
    float startY = rpY + panelPadY * sc;
    float scaledAvatarSize = avatarSize * sc;
    float scaledGap        = gap * sc;
    float scaledItemW      = itemW * sc;

    m_avatarRects.resize(n);

    for (int i = 0; i < n; ++i) {
        float ix = startX + i * (scaledItemW + scaledGap);
        float iy = startY;

        // Cache avatar rect for touch hit-testing
        m_avatarRects[i] = {ix, iy, scaledAvatarSize, scaledAvatarSize};

        // Move SelectionCursor to the selected avatar
        if (i == m_selected) {
            float pad = 4.f * sc;
            Rect cursorRect = {ix - pad, iy - pad,
                               scaledAvatarSize + pad * 2.f,
                               scaledAvatarSize + pad * 2.f};
            float circleRadius = cursorRect.width * 0.5f;
            m_cursor.moveTo(cursorRect, circleRadius, 0.15f);
        }

        // Avatar (circular = cornerRadius = half)
        auto& user = m_users[i];
        if (user.icon.valid()) {
            ren.drawTextureRounded(&user.icon,
                                   {ix, iy, scaledAvatarSize, scaledAvatarSize},
                                   scaledAvatarSize * 0.5f,
                                   Color::white().withAlpha(alpha));
        } else {
            // Placeholder circle
            float r = scaledAvatarSize * 0.5f;
            ren.drawCircle({ix + r, iy + r}, r,
                           Color(0.3f, 0.3f, 0.4f, 0.6f * alpha), 32);
        }

        // Touch-press highlight on non-selected avatar while finger is down
        if (i == m_touchHitAvatar && !m_touchOnSelected) {
            float r = scaledAvatarSize * 0.5f;
            ren.drawCircle({ix + r, iy + r}, r,
                           Color(1.f, 1.f, 1.f, 0.18f * alpha), 32);
        }

        // Name below avatar
        Font* f = m_smallFont ? m_smallFont : m_font;
        if (f && !user.nickname.empty()) {
            Vec2 nameSz = f->measure(user.nickname);
            float nameScale = 0.85f * sc;
            float actualW = nameSz.x * nameScale;
            // Centre under avatar
            float nx = ix + (scaledAvatarSize - actualW) * 0.5f;
            float ny = iy + scaledAvatarSize + 8.f * sc;
            Color nc = (i == m_selected) ? m_textPrimary : m_textSecondary;
            ren.drawText(user.nickname, {nx, ny}, f,
                         nc.withAlpha(alpha), nameScale);
        }
    }

    // Render the selection cursor on top
    m_cursor.render(ren);
}

} // namespace ui
