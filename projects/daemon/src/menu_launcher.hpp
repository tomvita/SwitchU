#pragma once
#include <switchu/smi_protocol.hpp>
#include <switchu/smi_helpers.hpp>
#include <switchu/file_log.hpp>
#include <switch.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace switchu::daemon::menu_la {

static AppletHolder g_holder = {};
static bool g_active = false;
static bool g_suspended = false;
static AppletId g_currentAppletId = AppletId_None;

inline bool isActive() {
    return g_active && appletHolderActive(&g_holder);
}

inline bool isSuspended() {
    return g_suspended;
}

inline void setSuspended(bool v) {
    g_suspended = v;
}

inline AppletId currentAppletId() {
    return g_currentAppletId;
}

// Slots where the menu should be kept alive across game launches and resumed
// (rather than terminated and relaunched) on Home-from-game. These slots are
// confirmed to survive am's suspend/resume cycle well and preserve menu state.
// Profile (MyPage) is intentionally excluded: user prefers fresh relaunch.
inline bool isHoldableSlot() {
    return g_currentAppletId == AppletId_LibraryAppletShop
        || g_currentAppletId == AppletId_LibraryAppletCabinet;
}

inline Result create() {
    switchu::FileLog::log("[menu_la] create() entry");
    AppletId appletId = AppletId_LibraryAppletPhotoViewer;  // default: Album (hbmenu)
    switchu::FileLog::log("[menu_la] probe: checking launch_profile flag");
    if (access(smi::kLaunchProfileFlag, F_OK) == 0) {
        appletId = AppletId_LibraryAppletMyPage;
        switchu::FileLog::log("[menu_la] launch_profile flag present, using MyPage applet");
    } else if (access(smi::kLaunchCabinetFlag, F_OK) == 0) {
        appletId = AppletId_LibraryAppletCabinet;
        switchu::FileLog::log("[menu_la] launch_cabinet flag present, using Cabinet applet");
    } else {
        switchu::FileLog::log("[menu_la] probe: checking launch_eshop flag");
        if (access(smi::kLaunchEshopFlag, F_OK) == 0) {
            appletId = AppletId_LibraryAppletShop;
            switchu::FileLog::log("[menu_la] launch_eshop flag present, using eShop applet");
        } else {
            switchu::FileLog::log("[menu_la] no flag, defaulting to Album applet");
        }
    }
    switchu::FileLog::log("[menu_la] probe: calling appletCreateLibraryApplet(id=0x%X)", (unsigned)appletId);
    Result rc = appletCreateLibraryApplet(&g_holder,
        appletId, LibAppletMode_AllForeground);
    switchu::FileLog::log("[menu_la] probe: appletCreateLibraryApplet returned 0x%X", rc);
    if (R_FAILED(rc)) {
        switchu::FileLog::log("[menu_la] CreateLibApplet FAIL: 0x%X", rc);
        return rc;
    }
    g_currentAppletId = appletId;
    return 0;
}

inline Result start(smi::MenuStartMode mode, const smi::SystemStatus& status) {
    LibAppletArgs la_args;
    libappletArgsCreate(&la_args, static_cast<u32>(mode));
    Result rc = libappletArgsPush(&la_args, &g_holder);
    if (R_FAILED(rc)) {
        switchu::FileLog::log("[menu_la] ArgsPush FAIL: 0x%X", rc);
        appletHolderClose(&g_holder);
        return rc;
    }

    AppletStorage st;
    rc = appletCreateStorage(&st, sizeof(status));
    if (R_SUCCEEDED(rc)) {
        appletStorageWrite(&st, 0, &status, sizeof(status));
        rc = appletHolderPushInData(&g_holder, &st);
        if (R_FAILED(rc)) {
            appletStorageClose(&st);
            switchu::FileLog::log("[menu_la] PushStatus FAIL: 0x%X", rc);
        }
    }

    rc = appletHolderStart(&g_holder);
    if (R_FAILED(rc)) {
        switchu::FileLog::log("[menu_la] Start FAIL: 0x%X", rc);
        appletHolderClose(&g_holder);
        return rc;
    }

    g_active = true;
    switchu::FileLog::log("[menu_la] started (mode=%u)", static_cast<u32>(mode));

    // PROBE: poll for up to 3s to see if am reports the applet finished immediately
    for (int i = 0; i < 30; i++) {
        svcSleepThread(100'000'000ULL); // 100ms
        if (appletHolderCheckFinished(&g_holder)) {
            LibAppletExitReason er = appletHolderGetExitReason(&g_holder);
            switchu::FileLog::log("[menu_la] probe: holder FINISHED early after %dms, exit_reason=%d",
                                  (i+1)*100, (int)er);
            return 0;
        }
    }
    switchu::FileLog::log("[menu_la] probe: holder still alive after 3s");
    return 0;
}

inline Result launch(smi::MenuStartMode mode, const smi::SystemStatus& status) {
    Result rc = create();
    if (R_FAILED(rc)) return rc;
    return start(mode, status);
}

inline void terminate() {
    if (!g_active) return;
    appletHolderRequestExitOrTerminate(&g_holder, 5'000'000'000ULL);
    appletHolderJoin(&g_holder);
    appletHolderClose(&g_holder);
    g_active = false;
    g_suspended = false;
    g_currentAppletId = AppletId_None;
}

inline bool checkFinished() {
    if (!g_active) return true;
    if (appletHolderCheckFinished(&g_holder)) {
        appletHolderJoin(&g_holder);
        appletHolderClose(&g_holder);
        g_active = false;
        g_suspended = false;
        g_currentAppletId = AppletId_None;
        return true;
    }
    return false;
}

inline LibAppletExitReason exitReason() {
    return appletHolderGetExitReason(&g_holder);
}

inline Result pushStorage(AppletStorage* st) {
    return appletHolderPushInteractiveInData(&g_holder, st);
}

inline Result popStorage(AppletStorage* st) {
    return appletHolderPopInteractiveOutData(&g_holder, st);
}

}
