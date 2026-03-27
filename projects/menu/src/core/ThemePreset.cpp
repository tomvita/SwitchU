#include "ThemePreset.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <algorithm>

nxui::Theme ThemePreset::toTheme() const {
    nxui::Theme t = (mode == nxui::ThemeMode::Dark)
                        ? nxui::Theme::dark()
                        : nxui::Theme::light();

    t.cursorNormal     = nxui::Color::fromHSL(colors.accentH, colors.accentS, colors.accentL, 1.f);
    float glowAlpha    = (mode == nxui::ThemeMode::Dark) ? 0.12f : 0.15f;
    t.cursorGlow       = t.cursorNormal.withAlpha(glowAlpha);

    t.background       = nxui::Color::fromHSL(colors.bgH, colors.bgS, colors.bgL, 1.f);
    t.backgroundAccent = nxui::Color::fromHSL(colors.bgAccH, colors.bgAccS, colors.bgAccL, 1.f);
    t.shapeColor       = nxui::Color::fromHSL(colors.shapeH, colors.shapeS, colors.shapeL, 0.10f);

    return t;
}

ThemeColorSet ThemePreset::extractColors(const nxui::Theme& theme) {
    ThemeColorSet c;
    theme.cursorNormal.toHSL(c.accentH, c.accentS, c.accentL);
    theme.background.toHSL(c.bgH, c.bgS, c.bgL);
    theme.backgroundAccent.toHSL(c.bgAccH, c.bgAccS, c.bgAccL);
    theme.shapeColor.toHSL(c.shapeH, c.shapeS, c.shapeL);
    return c;
}

static std::vector<ThemePreset> makeBuiltInPresets() {
    std::vector<ThemePreset> v;

    {
        ThemePreset p;
        p.name    = "Default Dark";
        p.mode    = nxui::ThemeMode::Dark;
        p.colors  = ThemePreset::extractColors(nxui::Theme::dark());
        p.builtIn = true;
        v.push_back(std::move(p));
    }

    {
        ThemePreset p;
        p.name    = "Default Light";
        p.mode    = nxui::ThemeMode::Light;
        p.colors  = ThemePreset::extractColors(nxui::Theme::light());
        p.builtIn = true;
        v.push_back(std::move(p));
    }

    {
        ThemePreset p;
        p.name    = "Ocean";
        p.mode    = nxui::ThemeMode::Dark;
        p.builtIn = true;
        p.colors.accentH  = 0.53f;  p.colors.accentS  = 0.80f; p.colors.accentL  = 0.55f;
        p.colors.bgH      = 0.58f;  p.colors.bgS      = 0.50f; p.colors.bgL      = 0.08f;
        p.colors.bgAccH   = 0.56f;  p.colors.bgAccS   = 0.55f; p.colors.bgAccL   = 0.14f;
        p.colors.shapeH   = 0.56f;  p.colors.shapeS   = 0.40f; p.colors.shapeL   = 0.30f;
        v.push_back(std::move(p));
    }

    {
        ThemePreset p;
        p.name    = "Sakura";
        p.mode    = nxui::ThemeMode::Light;
        p.builtIn = true;
        p.colors.accentH  = 0.94f;  p.colors.accentS  = 0.75f; p.colors.accentL  = 0.70f;
        p.colors.bgH      = 0.97f;  p.colors.bgS      = 0.30f; p.colors.bgL      = 0.94f;
        p.colors.bgAccH   = 0.94f;  p.colors.bgAccS   = 0.40f; p.colors.bgAccL   = 0.88f;
        p.colors.shapeH   = 0.94f;  p.colors.shapeS   = 0.35f; p.colors.shapeL   = 0.70f;
        v.push_back(std::move(p));
    }

    {
        ThemePreset p;
        p.name    = "Forest";
        p.mode    = nxui::ThemeMode::Dark;
        p.builtIn = true;
        p.colors.accentH  = 0.36f;  p.colors.accentS  = 0.65f; p.colors.accentL  = 0.45f;
        p.colors.bgH      = 0.39f;  p.colors.bgS      = 0.40f; p.colors.bgL      = 0.06f;
        p.colors.bgAccH   = 0.36f;  p.colors.bgAccS   = 0.45f; p.colors.bgAccL   = 0.10f;
        p.colors.shapeH   = 0.33f;  p.colors.shapeS   = 0.35f; p.colors.shapeL   = 0.25f;
        v.push_back(std::move(p));
    }

    {
        ThemePreset p;
        p.name    = "Sunset";
        p.mode    = nxui::ThemeMode::Dark;
        p.builtIn = true;
        p.colors.accentH  = 0.08f;  p.colors.accentS  = 0.85f; p.colors.accentL  = 0.60f;
        p.colors.bgH      = 0.03f;  p.colors.bgS      = 0.45f; p.colors.bgL      = 0.08f;
        p.colors.bgAccH   = 0.06f;  p.colors.bgAccS   = 0.50f; p.colors.bgAccL   = 0.14f;
        p.colors.shapeH   = 0.04f;  p.colors.shapeS   = 0.40f; p.colors.shapeL   = 0.28f;
        v.push_back(std::move(p));
    }

    return v;
}

const std::vector<ThemePreset>& ThemePreset::builtInPresets() {
    static std::vector<ThemePreset> presets = makeBuiltInPresets();
    return presets;
}

static constexpr const char* kUserPresetsPath = "sdmc:/config/SwitchU/theme_presets.ini";

std::vector<ThemePreset> ThemePreset::loadUserPresets() {
    std::vector<ThemePreset> result;
    std::ifstream f(kUserPresetsPath);
    if (!f.is_open()) return result;

    ThemePreset current;
    bool hasSection = false;

    std::string line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();

        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            if (hasSection)
                result.push_back(current);

            current = ThemePreset{};
            current.name = line.substr(1, line.size() - 2);
            current.builtIn = false;
            hasSection = true;
            continue;
        }

        if (!hasSection) continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        try {
            if      (key == "mode")      current.mode = (val == "light") ? nxui::ThemeMode::Light : nxui::ThemeMode::Dark;
            else if (key == "accent_h")  current.colors.accentH  = std::stof(val);
            else if (key == "accent_s")  current.colors.accentS  = std::stof(val);
            else if (key == "accent_l")  current.colors.accentL  = std::stof(val);
            else if (key == "bg_h")      current.colors.bgH      = std::stof(val);
            else if (key == "bg_s")      current.colors.bgS      = std::stof(val);
            else if (key == "bg_l")      current.colors.bgL      = std::stof(val);
            else if (key == "bg_acc_h")  current.colors.bgAccH   = std::stof(val);
            else if (key == "bg_acc_s")  current.colors.bgAccS   = std::stof(val);
            else if (key == "bg_acc_l")  current.colors.bgAccL   = std::stof(val);
            else if (key == "shape_h")   current.colors.shapeH   = std::stof(val);
            else if (key == "shape_s")   current.colors.shapeS   = std::stof(val);
            else if (key == "shape_l")   current.colors.shapeL   = std::stof(val);
        } catch (...) {}
    }

    if (hasSection)
        result.push_back(current);

    return result;
}

bool ThemePreset::saveUserPresets(const std::vector<ThemePreset>& presets) {
    mkdir("sdmc:/config", 0777);
    mkdir("sdmc:/config/SwitchU", 0777);

    FILE* f = fopen(kUserPresetsPath, "w");
    if (!f) return false;

    for (auto& p : presets) {
        fprintf(f, "[%s]\n", p.name.c_str());
        fprintf(f, "mode=%s\n", p.mode == nxui::ThemeMode::Light ? "light" : "dark");
        fprintf(f, "accent_h=%.4f\n", p.colors.accentH);
        fprintf(f, "accent_s=%.4f\n", p.colors.accentS);
        fprintf(f, "accent_l=%.4f\n", p.colors.accentL);
        fprintf(f, "bg_h=%.4f\n",     p.colors.bgH);
        fprintf(f, "bg_s=%.4f\n",     p.colors.bgS);
        fprintf(f, "bg_l=%.4f\n",     p.colors.bgL);
        fprintf(f, "bg_acc_h=%.4f\n", p.colors.bgAccH);
        fprintf(f, "bg_acc_s=%.4f\n", p.colors.bgAccS);
        fprintf(f, "bg_acc_l=%.4f\n", p.colors.bgAccL);
        fprintf(f, "shape_h=%.4f\n",  p.colors.shapeH);
        fprintf(f, "shape_s=%.4f\n",  p.colors.shapeS);
        fprintf(f, "shape_l=%.4f\n",  p.colors.shapeL);
        fprintf(f, "\n");
    }

    fclose(f);
    return true;
}
