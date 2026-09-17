#pragma once

#include <switch.h>

#include <cstddef>
#include <cstdint>

namespace switchu::daemon {

struct LibraryAppletInput {
    const void* data = nullptr;
    std::size_t size = 0;
};

struct LibraryAppletRequest {
    AppletId id = AppletId_None;
    const char* name = "LibraryApplet";
    std::uint32_t version = 0;
    bool pushCommonArgs = true;
    bool playStartupSound = true;
    const LibraryAppletInput* inputs = nullptr;
    std::size_t inputCount = 0;
    void* output = nullptr;
    std::size_t outputSize = 0;
    std::size_t* outputTransferSize = nullptr;
    // Asked once the applet has closed; returning true leaves the foreground
    // alone because the caller hands it straight to the application.
    bool (*skipForegroundRestore)() = nullptr;
    // Asked when an exit is requested; returning true closes the applet at
    // once instead of waiting for it to exit. An applet held in the background
    // can't answer an exit request, so waiting only delays the caller.
    bool (*terminateOnExit)() = nullptr;
    // Points at the running applet's holder between start and join, so the
    // pump can exchange interactive storages with it; null otherwise.
    AppletHolder** activeHolder = nullptr;
};

using LibraryAppletPump = void (*)();
using LibraryAppletExitRequested = bool (*)();

// Runs a foreground library applet without delegating its lifetime to a
// blocking libnx convenience wrapper. This keeps the daemon able to process
// HOME and guarantees join/close on every successful start path.
Result runLibraryApplet(const LibraryAppletRequest& request,
                        LibraryAppletPump pump,
                        LibraryAppletExitRequested exitRequested);

} // namespace switchu::daemon
