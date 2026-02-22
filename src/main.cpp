#include "app/WiiUMenuApp.hpp"
#include "core/DebugLog.hpp"
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>

// ---------------------------------------------------------------------------
// Tell libnx we are the HOME Menu (SystemApplet).
// libnx's default __appInit (which runs before main) reads this symbol
// and calls appletInitialize() with the right type.  We must NOT call
// appletInitialize / smInitialize / fsInitialize / hidInitialize again
// in main(), or the internal event handles get corrupted and we never
// receive system messages (Home button, overlays, …).
// ---------------------------------------------------------------------------
extern "C" {
    u32 __nx_applet_type = AppletType_SystemApplet;

    // Request ~208 MB of heap — matches DeltaLaunch.
    // Must be a multiple of 0x200000.
    size_t __nx_heap_size = 0xD000000;
}

// ---------------------------------------------------------------------------
// userAppInit / userAppExit — libnx hooks called by the default __appInit
// AFTER sm, applet, hid, time, fs, fsdev are already initialized.
// This is where DeltaLaunch initializes its extra services.
// ---------------------------------------------------------------------------
extern "C" void userAppInit(void) {
    plInitialize(PlServiceType_System);
    setInitialize();
    setsysInitialize();
    nsInitialize();
    accountInitialize(AccountServiceType_System);
    nssuInitialize();
    avmInitialize();
    psmInitialize();
    romfsInit();
}

extern "C" void userAppExit(void) {
    psmExit();
    avmExit();
    nssuExit();
    accountExit();
    nsExit();
    setsysExit();
    setExit();
    plExit();
    romfsExit();
}

int main(int argc, char* argv[]) {
    DebugLog::log("[main] applet config...");
    appletLoadAndApplyIdlePolicySettings();

    DebugLog::log("[main] SDL_Init...");
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        DebugLog::log("[main] SDL_Init FAILED: %s", SDL_GetError());

    DebugLog::log("[main] TTF_Init...");
    if (TTF_Init() < 0)
        DebugLog::log("[main] TTF_Init FAILED: %s", TTF_GetError());

    DebugLog::log("[main] creating app...");
    {
        ui::WiiUMenuApp app;
        if (app.initialize())
            app.run();
        DebugLog::log("[main] app.shutdown...");
        app.shutdown();
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}
