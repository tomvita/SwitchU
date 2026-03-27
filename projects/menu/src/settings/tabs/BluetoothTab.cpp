#include "TabBuilders.hpp"
#include <nxui/core/I18n.hpp>
#include <switch.h>

SettingsScreen::Tab settings::tabs::BluetoothTab::build(SettingsScreen& screen) {
    (void)screen;
    using Tab = SettingsScreen::Tab;
    using SettingItem = SettingsScreen::SettingItem;
    using ItemType = SettingsScreen::ItemType;
    auto& i18n = nxui::I18n::instance();
    Tab t;
    t.name = i18n.tr("settings.tabs.bluetooth", "Bluetooth");

    {
        SettingItem it; it.label = i18n.tr("settings.bluetooth.bluetooth", "Bluetooth"); it.type = ItemType::Toggle;
        bool val = true;
        setsysGetBluetoothEnableFlag(&val);
        it.boolVal = val;
        it.anim01 = val ? 1.f : 0.f;
        it.onChange = [](SettingItem& self) {
            setsysSetBluetoothEnableFlag(self.boolVal);
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.bluetooth.afh", "Bluetooth AFH"); it.type = ItemType::Toggle;
        bool val = true;
        setsysGetBluetoothAfhEnableFlag(&val);
        it.boolVal = val;
        it.anim01 = val ? 1.f : 0.f;
        it.onChange = [](SettingItem& self) {
            setsysSetBluetoothAfhEnableFlag(self.boolVal);
        };
        t.items.push_back(std::move(it));
    }

    return t;
}
