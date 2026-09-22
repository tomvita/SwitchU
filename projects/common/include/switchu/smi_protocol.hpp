#pragma once
#include <cstdint>
#include <cstring>

namespace switchu::smi {

static constexpr uint32_t kCommandMagic      = 0x53575543;
static constexpr uint32_t kStorageSize       = 0x8000;
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
    BatteryStatusChanged  = 10,
    OperationFailed       = 11,
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
    LaunchUserPage        = 14,
    LaunchControllerRemapping = 15,
    LaunchUserCreator     = 16,

    EnterSleep            = 20,
    Shutdown              = 21,
    Reboot                = 22,
    RequestForeground     = 23,

    GetAppList            = 30,
    GetSystemStatus       = 31,
    IsApplicationValid    = 32,
    SetManualDateTime     = 33,
    SetInternetTimeSync   = 34,

    MenuReady             = 40,
    MenuClosing           = 41,
};

enum class MenuStartMode : uint32_t {
    MainMenu       = 0,
    StartupBoot    = 1,
    AppletReturn   = 2,
};

struct CommandHeader {
    uint32_t magic;
    uint32_t message;
    uint64_t request_id;
};
static_assert(sizeof(CommandHeader) == 16);

struct LaunchAppArgs {
    uint64_t title_id;
    uint8_t  user_uid[16];
};
static_assert(sizeof(LaunchAppArgs) == 24);

struct UserArgs {
    uint8_t user_uid[16];
};
static_assert(sizeof(UserArgs) == 16);

struct ManualDateTimeArgs {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
};
static_assert(sizeof(ManualDateTimeArgs) == 20);

struct InternetTimeSyncArgs {
    uint8_t enabled;
    uint8_t _pad[7];
};
static_assert(sizeof(InternetTimeSyncArgs) == 8);

struct OperationOutcome {
    uint64_t request_id;
    uint64_t title_id;
    uint32_t command;
    uint32_t result;
};
static_assert(sizeof(OperationOutcome) == 24);

struct SystemStatus {
    uint64_t  suspended_app_id;
    uint8_t   selected_user[16];
    bool      app_running;
    uint8_t   _pad[7];
    OperationOutcome last_failure;
};
static_assert(sizeof(SystemStatus) == 56);

struct AppEntryHeader {
    uint64_t  title_id;
    uint32_t  name_len;
    uint32_t  icon_data_len;
    uint32_t  view_flags;
    uint8_t   startup_user_account;
    uint8_t   startup_user_account_option;
    uint8_t   startup_user_known;
    uint8_t   _pad;
};
static_assert(sizeof(AppEntryHeader) == 24);

static constexpr uint32_t kNotifyMagic = 0x53574E54;

struct DaemonNotification {
    uint32_t magic;
    MenuMessage msg;
    uint64_t  app_id;
    uint32_t  payload;
    uint32_t  _pad;
};

static constexpr uint32_t kBatteryPercentMask  = 0xFF;
static constexpr uint32_t kBatteryChargerShift = 8;
static constexpr uint32_t kBatteryChargerMask  = 0xFF << kBatteryChargerShift;

inline uint32_t makeBatteryPayload(uint32_t percentage, uint32_t chargerType) {
    if (percentage > 100)
        percentage = 100;
    return (percentage & kBatteryPercentMask)
        | ((chargerType & 0xFF) << kBatteryChargerShift);
}

inline uint32_t batteryPayloadPercentage(uint32_t payload) {
    return payload & kBatteryPercentMask;
}

inline uint32_t batteryPayloadChargerType(uint32_t payload) {
    return (payload & kBatteryChargerMask) >> kBatteryChargerShift;
}

inline bool batteryPayloadCharging(uint32_t payload) {
    return batteryPayloadChargerType(payload) != 0;
}

// App catalog file the daemon publishes and the menu reads at startup.
// A fresh catalog is staged at kAppCatalogTmpPath and swapped in by rename;
// the previous copy is kept at kAppCatalogBakPath for the duration of the
// swap so a reader landing in that window still finds a valid catalog.
static constexpr const char* kAppCatalogPath    = "sdmc:/config/SwitchU/applist.bin";
static constexpr const char* kAppCatalogTmpPath = "sdmc:/config/SwitchU/applist.tmp";
static constexpr const char* kAppCatalogBakPath = "sdmc:/config/SwitchU/applist.bak";

// Breeze Home toggle (fork-only). Both files belong to Breeze:
// - kBreezeHomeToggleFlag holds "keep" (Breeze stays alive behind the game) or
//   "restart" (Breeze closes on HOME and the next HOME relaunches it through the
//   profile takeover). A missing file turns the toggle off.
// - kBreezeRunningFlag is written when Breeze starts with the toggle on and
//   removed by its Exit button. The daemon also removes it once the applet has
//   closed, so a crash cannot leave it behind.
static constexpr const char* kBreezeHomeToggleFlag = "sdmc:/config/SwitchU/breeze_home_toggle";
static constexpr const char* kBreezeRunningFlag    = "sdmc:/config/SwitchU/breeze_running";
// Written by Breeze's SwitchU button just before it exits to hbmenu: the next
// HOME closes the applet and opens the SwitchU menu, whatever the toggle says.
// Removed on use and whenever a Breeze applet starts or closes.
static constexpr const char* kBreezeOpenMenuFlag   = "sdmc:/config/SwitchU/breeze_open_switchu";
// Breeze first (fork-only): with this file present, HOME is Breeze. The daemon
// opens Breeze at boot and wherever it would open the SwitchU menu, and reopens
// it when it closes with no game running. The SwitchU menu is reached only
// through Breeze's SwitchU button. Breeze starts through the User Page loader
// when installed (kBreezeUserPageLoaderPath), otherwise the Album slot borrows
// the loader shipped with the fork (kBreezeAlbumLoaderDir), like the menu does.
// The loader sits next to the daemon, not in switch/SwitchU, so a daemon-only
// install (no SwitchU menu) can still start Breeze.
static constexpr const char* kBreezeFirstFlag          = "sdmc:/config/SwitchU/breeze_first";
static constexpr const char* kBreezeNroPath            = "sdmc:/switch/Breeze/Breeze.nro";
static constexpr const char* kBreezeUserPageLoaderPath = "sdmc:/atmosphere/contents/0100000000001013/exefs.nsp";
static constexpr const char* kBreezeAlbumLoaderDir     = "/atmosphere/contents/0100000000001000/breeze_loader";
static constexpr const char* kBreezeAlbumLoaderMain    = "sdmc:/atmosphere/contents/0100000000001000/breeze_loader/main";
static constexpr const char* kMenuExecutablePath       = "sdmc:/switch/SwitchU/bin/menu/main";
// Written by this fork's daemon at boot (and by Breeze after installing the
// fork) so Breeze can tell the Home-toggle fork from upstream SwitchU.
static constexpr const char* kHomeToggleForkMarker = "sdmc:/config/SwitchU/home_toggle";
// Installed with each fork release (xmake.lua writes it): "interface=N" and
// "version=<release tag>". Breeze reads the interface level to decide whether
// the installed fork is new enough:
//   1 (no file, releases 1.2.0a-d): Home toggle No restart / Fast restart
//   2: Overlay mode (smi::BreezeOverlayCommand) and Breeze's SwitchU button
//   3: BreezeOverlayCommand Release / StateChanged (Breeze's Break and Trace)
static constexpr const char* kForkInfoPath = "sdmc:/switch/SwitchU/fork.txt";
// Overlay layer ids Breeze created and hasn't destroyed (same path as Breeze's
// OVERLAY_LAYERS_FILE). The layers are created for aruid 0, so no process owns
// them: when Breeze is terminated (sleep, or a game applet) the daemon has to
// destroy them, or a full-screen layer stays on the display.
static constexpr const char* kBreezeOverlayLayersPath = "sdmc:/config/SwitchU/breeze_overlay_layers";
// Power events, one fopen/fclose per line so a hang still leaves the trail.
static constexpr const char* kPowerLogPath = "sdmc:/config/SwitchU/power.log";

// Breeze overlay (fork-only). kBreezeHomeToggleFlag = "overlay" keeps Breeze
// alive behind the running game; HOME then shows and hides Breeze on its own
// display layer over the live game instead of switching the foreground.
// Breeze writes kBreezeOverlayCapability into kBreezeRunningFlag when it
// understands BreezeOverlayMessage, exchanged as 16-byte storages over the
// library applet's interactive in/out channel. Every command is answered with
// Ack (arg = the command).
//
// kBreezeRunningFlag holds space-separated capabilities with
// kBreezeOverlayCapability first, since older daemons only compare that prefix.
// kBreezeOverlayCapability2: Breeze understands Release and sends StateChanged.
static constexpr const char* kBreezeOverlayCapability = "overlay1";
static constexpr const char* kBreezeOverlayCapability2 = "overlay2";
static constexpr uint32_t kBreezeOverlayMagic = 0x564F5A42; // "BZOV"

enum class BreezeOverlayCommand : uint32_t {
    EnterHidden = 1,        // stop drawing to the applet window; the game is about to run
    Show = 2,               // draw over the game and take its input
    Hide = 3,               // give input back and clear the overlay
    EnterNormal = 4,        // Breeze is about to get the foreground back
    Release = 5,            // overlay2: the applet is about to be closed (sleep, the game's
                            // own applet); let the game run, give input back, drop the layer
    Ack = 0x81,             // Breeze -> daemon
    RequestForeground = 0x82, // Breeze -> daemon: exiting, bring the applet to the front
    StateChanged = 0x83,    // overlay2, Breeze -> daemon, not acked: Breeze showed itself
                            // (arg = Show, a breakpoint hit) or hid itself (arg = Hide)
};

struct BreezeOverlayMessage {
    uint32_t magic = kBreezeOverlayMagic;
    uint32_t command = 0;
    uint32_t arg = 0;
    uint32_t reserved = 0;
};

static constexpr uint64_t kMenuTakeoverProgramId = 0x010000000000100DULL;
static constexpr uint64_t kMenuProcessProgramId  = 0x010000000000FFFFULL;
static constexpr uint32_t kLdrAtmosRegisterExternalCode   = 65000;
static constexpr uint32_t kLdrAtmosUnregisterExternalCode = 65001;

}
