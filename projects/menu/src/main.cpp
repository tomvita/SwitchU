
#include "core/WiiUMenuApp.hpp"
#include "core/DebugLog.hpp"
#include <nxui/Application.hpp>
#include <nxui/core/Renderer.hpp>
#include <switchu/smi_protocol.hpp>
#include <switchu/file_log.hpp>
#include <nxtc.h>
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <cstring>
#include <memory>

extern "C" {
    u32 __nx_applet_type = AppletType_LibraryApplet;

    size_t __nx_heap_size = 0xFA00000;

    TimeServiceType __nx_time_service_type = TimeServiceType_Menu;

    void __libnx_init_time(void);

    void __nx_win_init(void);
    void __nx_win_exit(void);
}

extern "C" void __appInit(void) {
    Result rc;

    svcOutputDebugString("[SwitchU-menu] __appInit start", 30);

    rc = smInitialize();
    if (R_FAILED(rc)) diagAbortWithResult(MAKERESULT(Module_Libnx, 500));

    rc = fsInitialize();
    if (R_FAILED(rc)) diagAbortWithResult(MAKERESULT(Module_Libnx, 501));

    rc = appletInitialize();
    if (R_FAILED(rc)) {
        svcOutputDebugString("[SwitchU-menu] appletInitialize FAIL", 37);
        diagAbortWithResult(MAKERESULT(Module_Libnx, 502));
    }
    svcOutputDebugString("[SwitchU-menu] appletInitialize OK", 34);

    rc = hidInitialize();
    if (R_FAILED(rc)) svcOutputDebugString("[SwitchU-menu] hidInitialize FAIL", 33);

    timeInitialize();
    __libnx_init_time();
    setsysInitialize();
    setInitialize();
    plInitialize(PlServiceType_System);
    psmInitialize();
    lblInitialize();
    splInitialize();
    accountInitialize(AccountServiceType_System);

    {
        SetSysFirmwareVersion fw = {};
        if (R_SUCCEEDED(setsysGetFirmwareVersion(&fw)))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro) | BIT(31));
    }

    rc = fsdevMountSdmc();
    if (R_FAILED(rc)) {
        svcOutputDebugString("[SwitchU-menu] fsdevMountSdmc FAIL, retry", 42);
        svcSleepThread(100'000'000ULL);
        rc = fsdevMountSdmc();
    }

    switchu::FileLog::open("menu");
    DebugLog::openFileLog();
    DebugLog::log("[menu] __appInit complete (sd mount: 0x%X)", rc);

    rc = nsInitialize();
    if (R_FAILED(rc))
        DebugLog::log("[menu] nsInitialize FAILED: 0x%X", rc);
    else
        DebugLog::log("[menu] nsInitialize OK");

    __nx_win_init();
    DebugLog::log("[menu] __nx_win_init done");

    svcOutputDebugString("[SwitchU-menu] __appInit done", 29);
}

extern "C" void __appExit(void) {
    DebugLog::closeFileLog();
    switchu::FileLog::close();

    __nx_win_exit();

    nsExit();
    accountExit();
    splExit();
    lblExit();
    psmExit();
    plExit();

    hidExit();
    appletExit();

    setExit();
    setsysExit();
    timeExit();

    fsdevUnmountAll();
    fsExit();
    smExit();
}

static switchu::smi::MenuStartMode readStartMode() {
    LibAppletArgs args{};
    AppletStorage stor{};
    if (R_SUCCEEDED(appletPopInData(&stor))) {
        s64 sz = 0;
        appletStorageGetSize(&stor, &sz);
        if (sz >= (s64)sizeof(LibAppletArgs)) {
            appletStorageRead(&stor, 0, &args, sizeof(args));
        }
        appletStorageClose(&stor);
    }
    return static_cast<switchu::smi::MenuStartMode>(args.LaVersion);
}

static switchu::smi::SystemStatus readSystemStatus() {
    switchu::smi::SystemStatus status{};
    AppletStorage stor{};
    if (R_SUCCEEDED(appletPopInData(&stor))) {
        s64 sz = 0;
        appletStorageGetSize(&stor, &sz);
        if (sz >= (s64)sizeof(switchu::smi::SystemStatus)) {
            appletStorageRead(&stor, 0, &status, sizeof(status));
        }
        appletStorageClose(&stor);
    }
    return status;
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    DebugLog::log("[menu] main() entry");

    if (!nxtcInitialize())
        DebugLog::log("[menu] nxtc init failed (non-fatal)");

    auto startMode = readStartMode();
    auto sysStatus = readSystemStatus();

    DebugLog::log("[menu] start mode=%d  suspended=0x%016lX running=%d",
                  (int)startMode, sysStatus.suspended_app_id, sysStatus.app_running);

    {
        std::string sdPath = std::string(SD_ASSETS) + "/shaders/";
        nxui::Renderer::setShaderBasePath(sdPath);
        DebugLog::log("[menu] shader path: %s", sdPath.c_str());
    }

    DebugLog::log("[menu] SDL_Init...");
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        DebugLog::log("[menu] SDL_Init FAILED: %s", SDL_GetError());

    DebugLog::log("[menu] TTF_Init...");
    if (TTF_Init() < 0)
        DebugLog::log("[menu] TTF_Init FAILED: %s", TTF_GetError());

    DebugLog::log("[menu] creating app...");
    {
        nxui::Application app;
        auto activity = std::make_unique<WiiUMenuApp>();
        activity->setStartupStatus(sysStatus.suspended_app_id, sysStatus.app_running);
        app.setActivity(std::move(activity));
        DebugLog::log("[menu] app.initialize...");
        if (app.initialize()) {
            DebugLog::log("[menu] app.run...");
            app.run();
        } else {
            DebugLog::log("[menu] app.initialize FAILED");
        }
        DebugLog::log("[menu] app.shutdown...");
        app.shutdown();
    }

    TTF_Quit();
    SDL_Quit();
    nxtcExit();
    DebugLog::log("[menu] exit");
    return 0;
}
