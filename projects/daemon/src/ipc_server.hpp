#pragma once
#include <switchu/smi_protocol.hpp>
#include <switchu/file_log.hpp>
#include <switch.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>
#include <atomic>

namespace switchu::daemon::ipc {

static std::mutex g_queueMutex;
static std::vector<smi::MenuMessageContext> g_queue;

inline void pushMessage(smi::MenuMessage msg, uint64_t app_id = 0) {
    std::lock_guard<std::mutex> lk(g_queueMutex);
    smi::MenuMessageContext ctx{};
    ctx.msg = msg;
    ctx.app_id = app_id;
    g_queue.push_back(ctx);
    switchu::FileLog::log("[ipc] pushMessage msg=%u queueSize=%d",
        (unsigned)msg, (int)g_queue.size());
}

inline void pushMessageCtx(const smi::MenuMessageContext& ctx) {
    std::lock_guard<std::mutex> lk(g_queueMutex);
    g_queue.push_back(ctx);
}

inline bool popMessage(smi::MenuMessageContext& out) {
    std::lock_guard<std::mutex> lk(g_queueMutex);
    if (g_queue.empty()) return false;
    out = g_queue.front();
    g_queue.erase(g_queue.begin());
    return true;
}

#ifdef ENABLE_NRO_LAUNCHER
static std::atomic<bool> g_launchMenuRequested{false};
#endif

static Handle g_serverHandle = INVALID_HANDLE;
static Handle g_sessionHandle = INVALID_HANDLE;
static Thread g_serverThread = {};
static std::atomic<bool> g_serverRunning{false};


static void handleIpcRequest() {
    void* tls = armGetTls();
    HipcParsedRequest req = hipcParseRequest(tls);

    uint32_t cmd_id = 0;
    if (req.data.data_words && req.meta.num_data_words >= 4) {
        const uint32_t* words = req.data.data_words;
        if (words[0] == 0x49434653) {
            cmd_id = words[2];
        }
    }

    HipcRequest response = hipcMakeRequest(tls, HipcMetadata{
        .type = CmifCommandType_Request,
        .num_data_words = 0,
    });

    uint32_t* out = response.data_words;

    switch (cmd_id) {
    case 0: {
        HipcRequest resp = hipcMakeRequest(tls, HipcMetadata{
            .type = CmifCommandType_Request,
            .num_data_words = 4,
        });
        resp.data_words[0] = 0x4F434653;
        resp.data_words[1] = 0;
        resp.data_words[2] = 0;
        resp.data_words[3] = 0;
        break;
    }
    case 1: {
        smi::MenuMessageContext ctx{};
        bool got = popMessage(ctx);

        constexpr uint32_t kPayloadWords = (sizeof(smi::MenuMessageContext) + 3) / 4;
        constexpr uint32_t kTotalWords = 4 + kPayloadWords;

        HipcRequest resp = hipcMakeRequest(tls, HipcMetadata{
            .type = CmifCommandType_Request,
            .num_data_words = kTotalWords,
        });
        resp.data_words[0] = 0x4F434653;
        resp.data_words[1] = 0;

        if (got) {
            resp.data_words[2] = 0;
            resp.data_words[3] = 0;
            std::memcpy(&resp.data_words[4], &ctx, sizeof(ctx));
            switchu::FileLog::log("[ipc] TryPop delivered msg=%u", (unsigned)ctx.msg);
        } else {
            resp.data_words[2] = MAKERESULT(Module_Libnx, 0xFD);
            resp.data_words[3] = 0;
        }
        break;
    }
#ifdef ENABLE_NRO_LAUNCHER
    case 2: {
        g_launchMenuRequested.store(true);
        switchu::FileLog::log("[ipc] LaunchMenu requested");
        HipcRequest resp = hipcMakeRequest(tls, HipcMetadata{
            .type = CmifCommandType_Request,
            .num_data_words = 4,
        });
        resp.data_words[0] = 0x4F434653;
        resp.data_words[1] = 0;
        resp.data_words[2] = 0;
        resp.data_words[3] = 0;
        break;
    }
#endif
    default: {
        HipcRequest resp = hipcMakeRequest(tls, HipcMetadata{
            .type = CmifCommandType_Request,
            .num_data_words = 4,
        });
        resp.data_words[0] = 0x4F434653;
        resp.data_words[1] = 0;
        resp.data_words[2] = MAKERESULT(Module_Libnx, 0xFC);
        resp.data_words[3] = 0;
        break;
    }
    }
}

static void serverThreadFunc(void* arg) {
    (void)arg;

    while (g_serverRunning.load()) {
        Handle handles[2];
        int handleCount = 0;

        handles[handleCount++] = g_serverHandle;

        if (g_sessionHandle != INVALID_HANDLE)
            handles[handleCount++] = g_sessionHandle;

        s32 idx = -1;
        Result rc = svcReplyAndReceive(&idx, handles, handleCount,
                                        INVALID_HANDLE, 100'000'000ULL);

        if (rc == KERNELRESULT(TimedOut)) continue;
        if (R_FAILED(rc)) {
            if (g_sessionHandle != INVALID_HANDLE) {
                svcCloseHandle(g_sessionHandle);
                g_sessionHandle = INVALID_HANDLE;
            }
            continue;
        }

        if (idx < 0 || idx >= handleCount) continue;

        if (handles[idx] == g_serverHandle) {
            Handle newSession = INVALID_HANDLE;
            rc = svcAcceptSession(&newSession, g_serverHandle);
            if (R_SUCCEEDED(rc)) {
                if (g_sessionHandle != INVALID_HANDLE)
                    svcCloseHandle(g_sessionHandle);
                g_sessionHandle = newSession;
            }
        } else {
            handleIpcRequest();

            rc = svcReplyAndReceive(&idx, handles, 0,
                                     g_sessionHandle, 0);
        }
    }
}

inline Result startServer() {
    SmServiceName name = smEncodeName(smi::kPrivateServiceName);
    Result rc = smRegisterService(&g_serverHandle, name, false, 1);
    if (R_FAILED(rc)) {
        switchu::FileLog::log("[ipc] smRegisterService FAIL: 0x%X", rc);
        return rc;
    }

    g_serverRunning.store(true);

    rc = threadCreate(&g_serverThread, serverThreadFunc, nullptr,
                      nullptr, 0x4000, 0x2C, -2);
    if (R_FAILED(rc)) {
        switchu::FileLog::log("[ipc] threadCreate FAIL: 0x%X", rc);
        smUnregisterService(name);
        svcCloseHandle(g_serverHandle);
        return rc;
    }

    rc = threadStart(&g_serverThread);
    if (R_FAILED(rc)) {
        switchu::FileLog::log("[ipc] threadStart FAIL: 0x%X", rc);
        threadClose(&g_serverThread);
        smUnregisterService(name);
        svcCloseHandle(g_serverHandle);
        return rc;
    }

    switchu::FileLog::log("[ipc] service '%s' started", smi::kPrivateServiceName);
    return 0;
}

inline void stopServer() {
    g_serverRunning.store(false);
    threadWaitForExit(&g_serverThread);
    threadClose(&g_serverThread);

    if (g_sessionHandle != INVALID_HANDLE) {
        svcCloseHandle(g_sessionHandle);
        g_sessionHandle = INVALID_HANDLE;
    }

    SmServiceName name = smEncodeName(smi::kPrivateServiceName);
    smUnregisterService(name);
    svcCloseHandle(g_serverHandle);
    g_serverHandle = INVALID_HANDLE;

    switchu::FileLog::log("[ipc] service stopped");
}

}
