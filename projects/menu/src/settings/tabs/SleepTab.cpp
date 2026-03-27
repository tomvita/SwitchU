#include "TabBuilders.hpp"
#include <nxui/core/I18n.hpp>
#include <switch.h>
#include <algorithm>

SettingsScreen::Tab settings::tabs::SleepTab::build(SettingsScreen& screen) {
    (void)screen;
    using Tab = SettingsScreen::Tab;
    using SettingItem = SettingsScreen::SettingItem;
    using ItemType = SettingsScreen::ItemType;
    auto& i18n = nxui::I18n::instance();
    Tab t;
    t.name = i18n.tr("settings.tabs.sleep", "Sleep");

    const std::vector<std::string> handLabels = {
        i18n.tr("settings.sleep.1min", "1 min"),
        i18n.tr("settings.sleep.3min", "3 min"),
        i18n.tr("settings.sleep.5min", "5 min"),
        i18n.tr("settings.sleep.10min", "10 min"),
        i18n.tr("settings.sleep.30min", "30 min"),
        i18n.tr("settings.sleep.never", "Never")
    };
    const std::vector<std::string> dockLabels = {
        i18n.tr("settings.sleep.1h", "1 hour"),
        i18n.tr("settings.sleep.2h", "2 hours"),
        i18n.tr("settings.sleep.3h", "3 hours"),
        i18n.tr("settings.sleep.6h", "6 hours"),
        i18n.tr("settings.sleep.12h", "12 hours"),
        i18n.tr("settings.sleep.never", "Never")
    };

    SetSysSleepSettings sleep{};
    setsysGetSleepSettings(&sleep);

    {
        SettingItem it; it.label = i18n.tr("settings.sleep.handheld", "Handheld Auto-Sleep"); it.type = ItemType::Selector;
        it.description = i18n.tr("settings.sleep.handheld_desc", "Idle delay before automatic sleep in handheld mode.");
        it.options = handLabels;
        it.intVal = std::clamp((int)sleep.handheld_sleep_plan, 0, 5);
        it.onChange = [](SettingItem& self) {
            SetSysSleepSettings s{};
            setsysGetSleepSettings(&s);
            s.handheld_sleep_plan = std::clamp(self.intVal, 0, 5);
            setsysSetSleepSettings(&s);
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.sleep.docked", "Docked Auto-Sleep"); it.type = ItemType::Selector;
        it.description = i18n.tr("settings.sleep.docked_desc", "Idle delay before automatic sleep while docked.");
        it.options = dockLabels;
        it.intVal = std::clamp((int)sleep.console_sleep_plan, 0, 5);
        it.onChange = [](SettingItem& self) {
            SetSysSleepSettings s{};
            setsysGetSleepSettings(&s);
            s.console_sleep_plan = std::clamp(self.intVal, 0, 5);
            setsysSetSleepSettings(&s);
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.sleep.media_sleep", "Sleep While Playing Media"); it.type = ItemType::Toggle;
        it.description = i18n.tr("settings.sleep.media_sleep_desc", "Allow the console to auto-sleep even during media playback.");
        bool val = (sleep.flags & (1u << 0)) != 0;
        it.boolVal = val;
        it.anim01 = val ? 1.f : 0.f;
        it.onChange = [](SettingItem& self) {
            SetSysSleepSettings s{};
            setsysGetSleepSettings(&s);
            if (self.boolVal)
                s.flags |= (1u << 0);
            else
                s.flags &= ~(1u << 0);
            setsysSetSleepSettings(&s);
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.sleep.wake_power", "Wake at Power State Change"); it.type = ItemType::Toggle;
        it.description = i18n.tr("settings.sleep.wake_power_desc", "Wake the console when the AC adapter is connected or disconnected.");
        bool val = (sleep.flags & (1u << 1)) != 0;
        it.boolVal = val;
        it.anim01 = val ? 1.f : 0.f;
        it.onChange = [](SettingItem& self) {
            SetSysSleepSettings s{};
            setsysGetSleepSettings(&s);
            if (self.boolVal)
                s.flags |= (1u << 1);
            else
                s.flags &= ~(1u << 1);
            setsysSetSleepSettings(&s);
        };
        t.items.push_back(std::move(it));
    }

    return t;
}
