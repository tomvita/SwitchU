// ui/Theme.cpp
#include "Theme.hpp"

namespace ui {

Theme Theme::dark() {
    Theme t;
    t.mode              = ThemeMode::Dark;
    t.background        = Color(0.04f, 0.04f, 0.08f, 1.f);
    t.backgroundAccent  = Color(0.08f, 0.06f, 0.16f, 1.f);
    t.panelBase         = Color(0.14f, 0.14f, 0.22f, 0.40f);
    t.panelBorder       = Color(0.50f, 0.50f, 0.65f, 0.22f);
    t.panelHighlight    = Color(1.f, 1.f, 1.f, 0.05f);
    t.iconDefault       = Color(0.16f, 0.16f, 0.26f, 0.45f);
    t.cursorNormal      = Color(0.35f, 0.55f, 1.f, 1.f);
    t.cursorGlow        = Color(0.35f, 0.55f, 1.f, 0.12f);
    t.textPrimary       = Color(1.f, 1.f, 1.f, 1.f);
    t.textSecondary     = Color(0.70f, 0.72f, 0.80f, 0.80f);
    t.pageIndicator     = Color(1.f, 1.f, 1.f, 0.25f);
    t.pageIndicatorActive = Color(1.f, 1.f, 1.f, 1.f);
    t.shapeColor        = Color(0.22f, 0.18f, 0.38f, 0.10f);
    return t;
}

Theme Theme::light() {
    Theme t;
    t.mode              = ThemeMode::Light;
    t.background        = Color(0.92f, 0.93f, 0.96f, 1.f);
    t.backgroundAccent  = Color(0.85f, 0.88f, 0.95f, 1.f);
    t.panelBase         = Color(1.f, 1.f, 1.f, 0.48f);
    t.panelBorder       = Color(0.70f, 0.72f, 0.80f, 0.28f);
    t.panelHighlight    = Color(1.f, 1.f, 1.f, 0.35f);
    t.iconDefault       = Color(1.f, 1.f, 1.f, 0.50f);
    t.cursorNormal      = Color(0.0f, 0.48f, 1.f, 1.f);
    t.cursorGlow        = Color(0.0f, 0.48f, 1.f, 0.15f);
    t.textPrimary       = Color(0.10f, 0.10f, 0.15f, 1.f);
    t.textSecondary     = Color(0.30f, 0.30f, 0.40f, 0.85f);
    t.pageIndicator     = Color(0.30f, 0.30f, 0.40f, 0.30f);
    t.pageIndicatorActive = Color(0.10f, 0.10f, 0.20f, 1.f);
    t.shapeColor        = Color(0.55f, 0.58f, 0.80f, 0.10f);
    return t;
}

} // namespace ui
