#include "TabBuilders.hpp"
#include <nxui/core/I18n.hpp>
#include <switch.h>
#include <algorithm>

SettingsScreen::Tab settings::tabs::AudioTab::build(SettingsScreen& screen) {
    (void)screen;
    using Tab = SettingsScreen::Tab;
    using SettingItem = SettingsScreen::SettingItem;
    using ItemType = SettingsScreen::ItemType;
    auto& i18n = nxui::I18n::instance();
    Tab t;
    t.name = i18n.tr("settings.tabs.audio", "Audio");

    {
        SettingItem it; it.label = i18n.tr("settings.audio.speaker_auto_mute", "Speaker Auto-Mute"); it.type = ItemType::Toggle;
        it.description = i18n.tr("settings.audio.speaker_auto_mute_desc", "Mute speakers automatically when headphones are connected.");
        bool val = false;
        setsysGetSpeakerAutoMuteFlag(&val);
        it.boolVal = val;
        it.anim01 = val ? 1.f : 0.f;
        it.onChange = [](SettingItem& self) {
            setsysSetSpeakerAutoMuteFlag(self.boolVal);
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.audio.headphone_warning", "Headphone Volume Warning"); it.type = ItemType::Info;
        u32 count = 0;
        setsysGetHeadphoneVolumeWarningCount(&count);
        it.infoText = count > 0 ? i18n.tr("settings.audio.acknowledged", "Acknowledged")
                                : i18n.tr("common.active", "Active");
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.audio.tv_audio_output", "TV Audio Output"); it.type = ItemType::Selector;
        it.description = i18n.tr("settings.audio.tv_audio_output_desc", "Select audio format sent to TV over HDMI.");
        it.options = {
            i18n.tr("settings.audio.mono", "Mono"),
            i18n.tr("settings.audio.stereo", "Stereo"),
            i18n.tr("settings.audio.surround", "Surround")
        };

        SetSysAudioOutputMode mode = (SetSysAudioOutputMode)1;
        setsysGetAudioOutputMode(SetSysAudioOutputModeTarget_Unknown0, &mode);
        it.intVal = std::clamp((int)mode, 0, 2);
        it.onChange = [](SettingItem& self) {
            setsysSetAudioOutputMode(SetSysAudioOutputModeTarget_Unknown0,
                                     (SetSysAudioOutputMode)self.intVal);
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.audio.tv_resolution", "TV Resolution"); it.type = ItemType::Selector;
        it.description = i18n.tr("settings.audio.tv_resolution_desc", "Adjust HDMI resolution. Auto uses recommended value.");
        it.options = {
            i18n.tr("common.auto", "Auto"),
            i18n.tr("settings.audio.res_720p", "720p"),
            i18n.tr("settings.audio.res_1080p", "1080p")
        };
        SetSysTvSettings tv{};
        if (R_SUCCEEDED(setsysGetTvSettings(&tv)))
            it.intVal = std::clamp((int)tv.tv_resolution, 0, 2);
        it.onChange = [](SettingItem& self) {
            SetSysTvSettings tv{};
            if (R_SUCCEEDED(setsysGetTvSettings(&tv))) {
                tv.tv_resolution = (s32)self.intVal;
                setsysSetTvSettings(&tv);
            }
        };
        t.items.push_back(std::move(it));
    }

    {
        SettingItem it; it.label = i18n.tr("settings.audio.hdmi_rgb_range", "HDMI RGB Range"); it.type = ItemType::Selector;
        it.description = i18n.tr("settings.audio.hdmi_rgb_range_desc", "Full for PC monitors, Limited for most TVs.");
        it.options = {
            i18n.tr("common.auto", "Auto"),
            i18n.tr("settings.audio.full", "Full"),
            i18n.tr("settings.audio.limited", "Limited")
        };
        SetSysTvSettings tv{};
        if (R_SUCCEEDED(setsysGetTvSettings(&tv)))
            it.intVal = std::clamp((int)tv.rgb_range, 0, 2);
        it.onChange = [](SettingItem& self) {
            SetSysTvSettings tv{};
            if (R_SUCCEEDED(setsysGetTvSettings(&tv))) {
                tv.rgb_range = (s32)self.intVal;
                setsysSetTvSettings(&tv);
            }
        };
        t.items.push_back(std::move(it));
    }

    return t;
}
