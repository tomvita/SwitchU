#include "app/WiiUMenuApp.hpp"
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>

int main(int argc, char* argv[]) {
    socketInitializeDefault();
    nxlinkStdio();

    romfsInit();
    psmInitialize();
    nsInitialize();

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        std::printf("SDL_Init failed: %s\n", SDL_GetError());

    if (TTF_Init() < 0)
        std::printf("TTF_Init failed: %s\n", TTF_GetError());

    {
        ui::WiiUMenuApp app;
        if (app.initialize())
            app.run();
        app.shutdown();
    }

    TTF_Quit();
    SDL_Quit();
    nsExit();
    psmExit();
    romfsExit();
    std::fflush(stdout);
    socketExit();
    return 0;
}
