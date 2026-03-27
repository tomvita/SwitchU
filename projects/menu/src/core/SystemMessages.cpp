#include "SystemMessages.hpp"
#include "DebugLog.hpp"
#include <switch.h>
#ifdef SWITCHU_MENU
#include <switchu/smi_protocol.hpp>
#endif

#ifdef SWITCHU_HOMEBREW
#else

void SystemMessages::pushAction(SysAction a) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_pending.push_back(a);
}

void SystemMessages::pump() {
    std::vector<SysAction> actions;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        actions.swap(m_pending);
    }
    if (m_callback) {
        for (auto a : actions)
            m_callback(a);
    }
}

#ifdef SWITCHU_MENU

void SystemMessages::start() {
}

void SystemMessages::stop() {
}

#else

void SystemMessages::aeThreadFunc(void* arg) {
    auto* self = static_cast<SystemMessages*>(arg);
    while (self->m_running) {
        u32 msg = 0;
        if (R_SUCCEEDED(appletGetMessage(&msg))) {
            DebugLog::log("[ae] applet msg=%u", msg);
            switch (msg) {
                case 20:
                    appletRequestToGetForeground();
                    self->pushAction(SysAction::HomeButton);
                    break;
                case 22: case 29: case 32:
                    appletStartSleepSequence(true);
                    break;
                case 26:
                    DebugLog::log("[ae] -> Wakeup");
                    break;
                case AppletMessage_OperationModeChanged:
                case AppletMessage_PerformanceModeChanged:
                    break;
                default:
                    DebugLog::log("[ae] unhandled msg=%u", msg);
                    break;
            }
        }
        svcSleepThread(0);
    }
}

void SystemMessages::samsThreadFunc(void* arg) {
    auto* self = static_cast<SystemMessages*>(arg);
    while (self->m_running) {
        Event epop;
        Result rc = appletGetPopFromGeneralChannelEvent(&epop);
        if (R_FAILED(rc)) {
            svcSleepThread(1'000'000'000ULL);
            continue;
        }
        rc = eventWait(&epop, 1'000'000'000ULL);
        if (R_SUCCEEDED(rc)) {
            AppletStorage st;
            rc = appletPopFromGeneralChannel(&st);
            if (R_SUCCEEDED(rc)) {
                struct SamsHeader {
                    u32 magic, version, msg, reserved;
                } hdr = {};
                s64 stSize = 0;
                appletStorageGetSize(&st, &stSize);
                if (stSize > 0)
                    appletStorageRead(&st, 0, &hdr,
                        (size_t)stSize < sizeof(hdr) ? (size_t)stSize : sizeof(hdr));
                if (hdr.magic == 0x534D4153) {
                    switch (hdr.msg) {
                        case 2:
                            appletRequestToGetForeground();
                            self->pushAction(SysAction::HomeButton);
                            break;
                        case 3: appletStartSleepSequence(true); break;
                        case 5: appletStartShutdownSequence();  break;
                        case 6: appletStartRebootSequence();    break;
                        default: break;
                    }
                }
                appletStorageClose(&st);
            }
        }
        eventClose(&epop);
    }
}

void SystemMessages::start() {
    m_running = true;
    s32 core = (svcGetCurrentProcessorNumber() + 1) % 4;

    Result rc = threadCreate(&m_samsThread, samsThreadFunc, this,
                             nullptr, 0x20000, 0x3B, core);
    if (R_SUCCEEDED(rc)) threadStart(&m_samsThread);

    rc = threadCreate(&m_aeThread, aeThreadFunc, this,
                      nullptr, 0x20000, 0x3B, core);
    if (R_SUCCEEDED(rc)) threadStart(&m_aeThread);
}

void SystemMessages::stop() {
    m_running = false;
    threadWaitForExit(&m_samsThread);
    threadClose(&m_samsThread);
    threadWaitForExit(&m_aeThread);
    threadClose(&m_aeThread);
}

#endif
#endif
