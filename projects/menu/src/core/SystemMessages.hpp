#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <switch.h>

#ifndef SWITCHU_HOMEBREW
enum class SysAction {
    HomeButton,
    Sleep,
    Shutdown,
    Reboot,
    OperationModeChanged,
};
#endif

class SystemMessages {
public:
#ifndef SWITCHU_HOMEBREW
    using ActionCallback = std::function<void(SysAction)>;

    void setCallback(ActionCallback cb) { m_callback = std::move(cb); }

    void start();
    void stop();

    void pushAction(SysAction a);

    void pump();

private:
    ActionCallback              m_callback;
    std::mutex                  m_mutex;
    std::vector<SysAction>      m_pending;

#ifndef SWITCHU_MENU
    Thread           m_samsThread = {};
    Thread           m_aeThread   = {};
    std::atomic<bool> m_running{false};

    static void samsThreadFunc(void* arg);
    static void aeThreadFunc(void* arg);
#endif
#endif
};
