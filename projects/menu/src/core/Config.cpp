#include "Config.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <string>
#include <fstream>

bool AppConfig::load() {
    std::ifstream f(kConfigPath);
    if (!f.is_open()) return false;

    bool hadDarkMode = false;
    bool darkModeVal = false;
    bool hadThemePreset = false;

    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        try {
            if      (key == "music_enabled") musicEnabled = std::stoi(val) != 0;
            else if (key == "music_volume")  musicVolume  = std::stof(val);
            else if (key == "sfx_volume")    sfxVolume    = std::stof(val);
            else if (key == "dark_mode")     { hadDarkMode = true; darkModeVal = std::stoi(val) != 0; }
            else if (key == "ui_language_override") uiLanguageOverride = val;
            else if (key == "sound_preset") soundPreset = val;
            else if (key == "theme_preset") { themePreset = val; hadThemePreset = true; }
            else if (key == "theme_mode") themeMode = val;
            else if (key == "accent_h")  accentH = std::stof(val);
            else if (key == "accent_s")  accentS = std::stof(val);
            else if (key == "accent_l")  accentL = std::stof(val);
            else if (key == "bg_h")      bgH     = std::stof(val);
            else if (key == "bg_s")      bgS     = std::stof(val);
            else if (key == "bg_l")      bgL     = std::stof(val);
            else if (key == "bg_acc_h")  bgAccH  = std::stof(val);
            else if (key == "bg_acc_s")  bgAccS  = std::stof(val);
            else if (key == "bg_acc_l")  bgAccL  = std::stof(val);
            else if (key == "shape_h")   shapeH  = std::stof(val);
            else if (key == "shape_s")   shapeS  = std::stof(val);
            else if (key == "shape_l")   shapeL  = std::stof(val);
        } catch (...) {
        }
    }

    f.close();

    if (!hadThemePreset && hadDarkMode) {
        themePreset = darkModeVal ? "Default Dark" : "Default Light";
    }

    if (musicVolume < 0.f) musicVolume = 0.f;
    if (musicVolume > 1.f) musicVolume = 1.f;
    if (sfxVolume   < 0.f) sfxVolume   = 0.f;
    if (sfxVolume   > 1.f) sfxVolume   = 1.f;
    if (uiLanguageOverride.empty()) uiLanguageOverride = "auto";
    if (soundPreset.empty()) soundPreset = "wiiu";
    if (themePreset.empty()) themePreset = "Default Dark";

    auto clampHSL = [](float& v) { if (v < 0.f) v = -1.f; else if (v > 1.f) v = 1.f; };
    clampHSL(accentH); clampHSL(accentS); clampHSL(accentL);
    clampHSL(bgH);     clampHSL(bgS);     clampHSL(bgL);
    clampHSL(bgAccH);  clampHSL(bgAccS);  clampHSL(bgAccL);
    clampHSL(shapeH);  clampHSL(shapeS);  clampHSL(shapeL);

    return true;
}

bool AppConfig::save() const {
    mkdir("sdmc:/config", 0777);
    mkdir(kConfigDir, 0777);

    FILE* f = fopen(kConfigPath, "w");
    if (!f) return false;

    fprintf(f, "music_enabled=%d\n", musicEnabled ? 1 : 0);
    fprintf(f, "music_volume=%.2f\n", musicVolume);
    fprintf(f, "sfx_volume=%.2f\n",   sfxVolume);
    fprintf(f, "ui_language_override=%s\n", uiLanguageOverride.c_str());
    fprintf(f, "sound_preset=%s\n", soundPreset.c_str());
    fprintf(f, "theme_preset=%s\n", themePreset.c_str());
    if (!themeMode.empty()) fprintf(f, "theme_mode=%s\n", themeMode.c_str());

    if (accentH >= 0.f) fprintf(f, "accent_h=%.4f\n", accentH);
    if (accentS >= 0.f) fprintf(f, "accent_s=%.4f\n", accentS);
    if (accentL >= 0.f) fprintf(f, "accent_l=%.4f\n", accentL);
    if (bgH     >= 0.f) fprintf(f, "bg_h=%.4f\n",     bgH);
    if (bgS     >= 0.f) fprintf(f, "bg_s=%.4f\n",     bgS);
    if (bgL     >= 0.f) fprintf(f, "bg_l=%.4f\n",     bgL);
    if (bgAccH  >= 0.f) fprintf(f, "bg_acc_h=%.4f\n", bgAccH);
    if (bgAccS  >= 0.f) fprintf(f, "bg_acc_s=%.4f\n", bgAccS);
    if (bgAccL  >= 0.f) fprintf(f, "bg_acc_l=%.4f\n", bgAccL);
    if (shapeH  >= 0.f) fprintf(f, "shape_h=%.4f\n",  shapeH);
    if (shapeS  >= 0.f) fprintf(f, "shape_s=%.4f\n",  shapeS);
    if (shapeL  >= 0.f) fprintf(f, "shape_l=%.4f\n",  shapeL);

    fclose(f);
    return true;
}
