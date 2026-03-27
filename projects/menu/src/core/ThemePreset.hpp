#pragma once
#include <nxui/Theme.hpp>
#include <string>
#include <vector>

struct ThemeColorSet {
    float accentH = 0.f, accentS = 0.f, accentL = 0.f;
    float bgH = 0.f, bgS = 0.f, bgL = 0.f;
    float bgAccH = 0.f, bgAccS = 0.f, bgAccL = 0.f;
    float shapeH = 0.f, shapeS = 0.f, shapeL = 0.f;
};

struct ThemePreset {
    std::string     name;
    nxui::ThemeMode mode   = nxui::ThemeMode::Dark;
    ThemeColorSet   colors;
    bool            builtIn = true;

    nxui::Theme toTheme() const;

    static ThemeColorSet extractColors(const nxui::Theme& theme);

    static const std::vector<ThemePreset>& builtInPresets();

    static std::vector<ThemePreset> loadUserPresets();

    static bool saveUserPresets(const std::vector<ThemePreset>& presets);
};
