// SwitchU Launcher — minimal NRO
// Connects to the SwitchU daemon via IPC and requests it to open the menu.

#include <switch.h>
#include <cstring>

extern "C" {
    u32 __nx_applet_type = AppletType_Application;
    size_t __nx_heap_size = 0x200000; // 2 MB — plenty for this tiny app
}

static Service g_srv = {};

static bool connectToService() {
    Result rc = smGetService(&g_srv, "swu:m");
    return R_SUCCEEDED(rc);
}

static bool sendLaunchMenu() {
    // Build a raw HIPC request: 4 data words
    // word[0] = SFCI magic (0x49434653)
    // word[1] = 0
    // word[2] = cmd_id = 2 (LaunchMenu)
    // word[3] = 0
    HipcMetadata meta{};
    meta.type = CmifCommandType_Request;
    meta.num_data_words = 4;

    void* tls = armGetTls();
    HipcRequest req = hipcMakeRequest(tls, meta);
    req.data_words[0] = 0x49434653; // SFCI
    req.data_words[1] = 0;
    req.data_words[2] = 2;          // LaunchMenu
    req.data_words[3] = 0;

    Result rc = svcSendSyncRequest(g_srv.session);
    if (R_FAILED(rc)) return false;

    // Parse response — check for SFCO magic and rc == 0
    HipcParsedRequest resp = hipcParseRequest(tls);
    if (resp.data.data_words && resp.meta.num_data_words >= 4) {
        if (resp.data.data_words[0] == 0x4F434653 && resp.data.data_words[2] == 0)
            return true;
    }
    return false;
}

extern "C" void __appInit(void) {
    smInitialize();
}

extern "C" void __appExit(void) {
    serviceClose(&g_srv);
    smExit();
}

int main(int argc, char* argv[]) {
    // Try connecting with retries — daemon IPC server may take a moment
    bool connected = false;
    for (int i = 0; i < 50; ++i) {
        if (connectToService()) {
            connected = true;
            break;
        }
        svcSleepThread(100'000'000ULL); // 100ms
    }

    if (connected) {
        sendLaunchMenu();
    }

    return 0;
}
