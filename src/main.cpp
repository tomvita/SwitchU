#include "core/WiiUMenuApp.hpp"
#include "core/DebugLog.hpp"
#include <nxui/Application.hpp>
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>

extern "C" {
#ifdef SWITCHU_HOMEBREW
    u32 __nx_applet_type = AppletType_Application;
#else
    u32 __nx_applet_type = AppletType_SystemApplet;
#endif

    // Request ~208 MB of heap (multiple of 0x200000).
    size_t __nx_heap_size = 0xD000000;
}

extern "C" void userAppInit(void) {
    timeInitialize();
    plInitialize(PlServiceType_System);
    setInitialize();
    setsysInitialize();
#ifndef SWITCHU_HOMEBREW
    nsInitialize();
    accountInitialize(AccountServiceType_System);
    nssuInitialize();
    avmInitialize();
#else
    accountInitialize(AccountServiceType_Application);
#endif
    psmInitialize();
    lblInitialize();
    romfsInit();
}

extern "C" void userAppExit(void) {
    lblExit();
    psmExit();
#ifndef SWITCHU_HOMEBREW
    avmExit();
    nssuExit();
#endif
    accountExit();
#ifndef SWITCHU_HOMEBREW
    nsExit();
#endif
    setsysExit();
    setExit();
    plExit();
    timeExit();
    romfsExit();
}

int main(int argc, char* argv[]) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    DebugLog::log("[main] applet config...");
#ifndef SWITCHU_HOMEBREW
    appletLoadAndApplyIdlePolicySettings();
#endif

    DebugLog::log("[main] SDL_Init...");
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        DebugLog::log("[main] SDL_Init FAILED: %s", SDL_GetError());

    DebugLog::log("[main] TTF_Init...");
    if (TTF_Init() < 0)
        DebugLog::log("[main] TTF_Init FAILED: %s", TTF_GetError());

    DebugLog::log("[main] creating app...");
    {
        nxui::Application app;
        app.setActivity(std::make_unique<WiiUMenuApp>());
        if (app.initialize())
            app.run();
        DebugLog::log("[main] app.shutdown...");
        app.shutdown();
    }

    TTF_Quit();
    SDL_Quit();
    return 0;
}
