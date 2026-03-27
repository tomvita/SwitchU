#pragma once

#include "SettingsScreen.hpp"
#include <nxui/widgets/Box.hpp>
#include <memory>

namespace settings::widgets {

struct SettingWidgetContext {
    nxui::Font** font = nullptr;
    nxui::Font** smallFont = nullptr;
    const nxui::Theme** theme = nullptr;
};

std::shared_ptr<nxui::Box> createSettingItemWidget(SettingsScreen::SettingItem& item,
                                                    const SettingWidgetContext& ctx);

}
