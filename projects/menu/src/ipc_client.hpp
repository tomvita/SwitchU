#pragma once
#include <switchu/smi_protocol.hpp>
#include <switch.h>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include "core/DebugLog.hpp"

namespace switchu::menu::ipc {

using MessageCallback = std::function<void(const smi::MenuMessageContext&)>;

static Service g_service = {};
static Thread g_receiverThread = {};
static std::atomic<bool> g_running{false};

static std::mutex g_cbMutex;
static std::vector<std::pair<MessageCallback, smi::MenuMessage>> g_callbacks;


static Result cmdInitialize() {
    return serviceDispatch(&g_service, 0);
}

static Result cmdTryPopMessage(smi::MenuMessageContext& out) {
    return serviceDispatchOut(&g_service, 1, out);
}


static void receiverFunc(void* arg) {
    (void)arg;
    DebugLog::log("[ipc-client] receiver thread started");
    while (g_running.load()) {
        smi::MenuMessageContext ctx{};
        Result rc = cmdTryPopMessage(ctx);
        if (R_SUCCEEDED(rc)) {
            DebugLog::log("[ipc-client] received msg=%u", (unsigned)ctx.msg);
            std::lock_guard<std::mutex> lk(g_cbMutex);
            for (auto& [cb, filter] : g_callbacks) {
                if (filter == smi::MenuMessage::Invalid || filter == ctx.msg) {
                    cb(ctx);
                }
            }
        }
        svcSleepThread(10'000'000ULL);
    }
    DebugLog::log("[ipc-client] receiver thread exiting");
}


inline Result connect() {
    Result rc = smGetService(&g_service, smi::kPrivateServiceName);
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[ipc-client] smGetService FAIL: 0x%X\n", rc);
        return rc;
    }

    rc = cmdInitialize();
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[ipc-client] Initialize FAIL: 0x%X\n", rc);
        serviceClose(&g_service);
        return rc;
    }

    g_running.store(true);
    rc = threadCreate(&g_receiverThread, receiverFunc, nullptr,
                      nullptr, 0x4000, 0x2C, -2);
    if (R_SUCCEEDED(rc)) {
        threadStart(&g_receiverThread);
        DebugLog::log("[ipc-client] receiver thread launched");
    } else {
        DebugLog::log("[ipc-client] threadCreate FAIL: 0x%X", rc);
    }

    std::fprintf(stderr, "[ipc-client] connected to '%s'\n", smi::kPrivateServiceName);
    return 0;
}

inline void disconnect() {
    g_running.store(false);
    threadWaitForExit(&g_receiverThread);
    threadClose(&g_receiverThread);
    serviceClose(&g_service);
    {
        std::lock_guard<std::mutex> lk(g_cbMutex);
        g_callbacks.clear();
    }
    std::fprintf(stderr, "[ipc-client] disconnected\n");
}

inline void onMessage(smi::MenuMessage filter, MessageCallback cb) {
    std::lock_guard<std::mutex> lk(g_cbMutex);
    g_callbacks.emplace_back(std::move(cb), filter);
}

inline void onAnyMessage(MessageCallback cb) {
    onMessage(smi::MenuMessage::Invalid, std::move(cb));
}

}
