#pragma once
#include <cstdint>
#include <cstring>

namespace switchu::smi {

static constexpr uint32_t kCommandMagic      = 0x53575543;
static constexpr uint32_t kWakeMagic         = 0x57414B45;
static constexpr uint32_t kStorageSize       = 0x8000;

struct WakeSignal {
    uint32_t magic;
    uint32_t reason;
    uint64_t suspended_tid;
};
static_assert(sizeof(WakeSignal) == 16);
static constexpr uint32_t kMaxRetries        = 5000;
static constexpr uint64_t kRetrySleepNs      = 10'000'000;

enum class MenuMessage : uint32_t {
    Invalid               =  0,
    HomeRequest           =  1,
    ApplicationExited     =  2,
    ApplicationSuspended  =  3,
    SdCardEjected         =  4,
    AppRecordsChanged     =  5,
    SleepSequence         =  6,
    WakeUp                =  7,
    GameCardMountFailure  =  8,
    AppViewFlagsUpdate    =  9,
};

enum class SystemMessage : uint32_t {
    Invalid               =  0,

    LaunchApplication     =  1,
    ResumeApplication     =  2,
    TerminateApplication  =  3,

    LaunchAlbum           = 10,
    LaunchMiiEditor       = 11,
    LaunchControllers     = 12,
    LaunchNetConnect      = 13,

    EnterSleep            = 20,
    Shutdown              = 21,
    Reboot                = 22,
    RequestForeground     = 23,

    GetAppList            = 30,
    GetSystemStatus       = 31,
    IsApplicationValid    = 32,

    MenuReady             = 40,
    MenuClosing           = 41,
    MenuSuspending        = 42,
};

enum class MenuStartMode : uint32_t {
    MainMenu       = 0,
    StartupBoot    = 1,
    Resume         = 2,
};

struct CommandHeader {
    uint32_t magic;
    uint32_t message;
};
static_assert(sizeof(CommandHeader) == 8);

struct LaunchAppArgs {
    uint64_t title_id;
    uint8_t  user_uid[16];
};
static_assert(sizeof(LaunchAppArgs) == 24);

struct SystemStatus {
    uint64_t  suspended_app_id;
    uint8_t   selected_user[16];
    bool      app_running;
    uint8_t   _pad[7];
};
static_assert(sizeof(SystemStatus) == 32);

struct AppEntryHeader {
    uint64_t  title_id;
    uint32_t  name_len;
    uint32_t  icon_data_len;
    uint32_t  view_flags;
    uint32_t  _pad;
};
static_assert(sizeof(AppEntryHeader) == 24);

static constexpr const char* kPrivateServiceName = "swu:m";

struct MenuMessageContext {
    MenuMessage msg;
    union {
        uint64_t  app_id;
        struct { uint32_t mount_rc; } gc_mount_failure;
        uint32_t  raw[16];
    };
};
static_assert(sizeof(MenuMessageContext) <= 72);

static constexpr uint32_t kNotifyMagic = 0x53574E54;

struct DaemonNotification {
    uint32_t magic;
    MenuMessage msg;
    uint64_t  app_id;
    uint32_t  payload;
    uint32_t  _pad;
};

enum class PrivateServiceCmd : uint32_t {
    Initialize     = 0,
    TryPopMessage  = 1,
    LaunchMenu     = 2,
};
static constexpr uint64_t kEShopProgramId = 0x010000000000100BULL;

// Flag file written by Breeze to indicate it wants to be the Home-toggle target.
// When present, pressing Home while a game is running launches Breeze (Album slot)
// instead of the SwitchU menu. Breeze removes this file on "Exit" so Home reverts
// to the SwitchU menu.
static constexpr const char* kBreezeActiveFlag   = "sdmc:/config/SwitchU/breeze_active";
// Flag file written by Breeze when "Goto Game" is pressed. Tells the daemon to
// resume the suspended game instead of relaunching the menu after Breeze exits.
static constexpr const char* kBreezeGotoGameFlag = "sdmc:/config/SwitchU/breeze_goto_game";
// Menu applet selection — determined by flag files.
// Priority: launch_profile > launch_auth > launch_cabinet > launch_eshop > Album (default).
// When no flag is present the daemon launches Album (hbmenu).
//
// NOTE: launch_eshop (LibraryAppletShop, 0x010000000000100B) is BROKEN on HOS
// 22.1+ and cannot be fixed from homebrew. Although our menu boots into the
// Shop slot successfully, the LibAppletShop flow on HOS 22.1 immediately spawns
// Nintendo's systemWeb process (0x0100000000001042) as a child to host the
// storefront. systemWeb hits an internal precondition check, calls
// fatalThrow(0x4A2) — user sees Atmosphère crash screen "Error Code
// 2162-0002". This is Nintendo code we cannot patch.
//
// Working slot alternatives on HOS 22+:
//   launch_profile  — MyPage applet  (0x0100000000001013)
//   launch_auth     — Parental Controls PIN applet (0x0100000000001001)
//   launch_cabinet  — amiibo Cabinet applet (0x0100000000001002)
// Using Auth disables Parental Controls PIN entry; only use it on consoles
// where Parental Controls are not in use.
static constexpr const char* kLaunchProfileFlag  = "sdmc:/config/SwitchU/launch_profile";
static constexpr const char* kLaunchAuthFlag     = "sdmc:/config/SwitchU/launch_auth";
static constexpr const char* kLaunchCabinetFlag  = "sdmc:/config/SwitchU/launch_cabinet";
static constexpr const char* kLaunchEshopFlag    = "sdmc:/config/SwitchU/launch_eshop";
static constexpr uint32_t kLdrAtmosRegisterExternalCode   = 65000;
static constexpr uint32_t kLdrAtmosUnregisterExternalCode = 65001;

}
