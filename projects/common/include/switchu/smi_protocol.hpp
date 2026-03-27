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
    LaunchEShop           = 11,
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
};
static constexpr uint64_t kEShopProgramId = 0x010000000000100BULL;
static constexpr uint32_t kLdrAtmosRegisterExternalCode   = 65000;
static constexpr uint32_t kLdrAtmosUnregisterExternalCode = 65001;

}
