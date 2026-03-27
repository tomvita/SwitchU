#include "TabBuilders.hpp"
#include <nxui/core/I18n.hpp>

SettingsScreen::Tab settings::tabs::ThemeTab::build(SettingsScreen& screen) {
    using Tab = SettingsScreen::Tab;
    using SettingItem = SettingsScreen::SettingItem;
    using ItemType = SettingsScreen::ItemType;
    auto& i18n = nxui::I18n::instance();
    Tab t;
    t.name = i18n.tr("settings.tabs.theme", "Theme");

    const auto& c = screen.m_themeColors;

    {
        SettingItem it;
        it.label   = i18n.tr("settings.theme.preset", "Preset");
        it.type    = ItemType::Selector;
        it.options = screen.m_themePresetNames;
        it.intVal  = 0;
        for (int i = 0; i < (int)screen.m_themePresetNames.size(); ++i) {
            if (screen.m_themePresetNames[i] == screen.m_themePresetName) {
                it.intVal = i;
                break;
            }
        }
        it.onChange = [&screen](SettingItem& self) {
            if (self.intVal >= 0 && self.intVal < (int)self.options.size()) {
                if (screen.m_themePresetChangeCb)
                    screen.m_themePresetChangeCb(self.options[self.intVal]);
            }
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it;
        it.label   = i18n.tr("settings.theme.menu_mode", "Menu Mode");
        it.type    = ItemType::Selector;
        it.options = {
            i18n.tr("settings.theme.mode_dark", "Dark"),
            i18n.tr("settings.theme.mode_light", "Light")
        };
        it.intVal = screen.m_themeModeDark ? 0 : 1;
        it.onChange = [&screen](SettingItem& self) {
            bool dark = (self.intVal == 0);
            if (screen.m_themeModeCb) screen.m_themeModeCb(dark);
        };
        t.items.push_back(std::move(it));
    }

    auto makeColorPicker = [&](const std::string& label, float h, float s, float l,
                                const std::string& hKey, const std::string& sKey, const std::string& lKey) {
        SettingItem it;
        it.label  = label;
        it.type   = ItemType::ColorPicker;
        it.colorH = h;
        it.colorS = s;
        it.colorL = l;
        it.options = { hKey, sKey, lKey };
        it.onChange = [&screen](SettingItem& self) {
            if (screen.m_themeColorChangeCb && self.options.size() >= 3) {
                screen.m_themeColorChangeCb(self.options[0], self.colorH);
                screen.m_themeColorChangeCb(self.options[1], self.colorS);
                screen.m_themeColorChangeCb(self.options[2], self.colorL);
            }
        };
        return it;
    };

    t.items.push_back(makeColorPicker(
        i18n.tr("settings.theme.accent", "Accent"),
        c.accentH, c.accentS, c.accentL,
        "accent_h", "accent_s", "accent_l"));

    t.items.push_back(makeColorPicker(
        i18n.tr("settings.theme.background", "Background"),
        c.bgH, c.bgS, c.bgL,
        "bg_h", "bg_s", "bg_l"));

    t.items.push_back(makeColorPicker(
        i18n.tr("settings.theme.bg_accent", "Background Accent"),
        c.bgAccH, c.bgAccS, c.bgAccL,
        "bg_acc_h", "bg_acc_s", "bg_acc_l"));

    t.items.push_back(makeColorPicker(
        i18n.tr("settings.theme.shapes", "Floating Shapes"),
        c.shapeH, c.shapeS, c.shapeL,
        "shape_h", "shape_s", "shape_l"));

    {
        SettingItem it;
        it.label = i18n.tr("settings.theme.delete_preset", "Delete Preset");
        it.type  = ItemType::Action;
        it.onChange = [&screen](SettingItem&) {
            if (screen.m_themeManageCb) screen.m_themeManageCb();
        };
        t.items.push_back(std::move(it));
    }
    {
        SettingItem it;
        it.label = i18n.tr("settings.theme.save_preset", "Save as Preset");
        it.type  = ItemType::Action;
        it.onChange = [&screen](SettingItem&) {
            if (screen.m_themeSaveCb) screen.m_themeSaveCb();
        };
        t.items.push_back(std::move(it));
    }
    {
        SettingItem it;
        it.label = i18n.tr("settings.theme.reset_defaults", "Reset to Defaults");
        it.type  = ItemType::Action;
        it.onChange = [&screen](SettingItem&) {
            if (screen.m_themeResetCb) screen.m_themeResetCb();
        };
        t.items.push_back(std::move(it));
    }

    return t;
}
