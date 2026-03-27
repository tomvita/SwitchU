#pragma once
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <sys/stat.h>
#include <switch/kernel/svc.h>

namespace switchu {

class FileLog {
public:
    static constexpr const char* LOG_DIR = "sdmc:/config/SwitchU";

    static void open(const char* tag) {
        ::mkdir("sdmc:/config", 0755);
        ::mkdir(LOG_DIR, 0755);
        char path[128];
        std::snprintf(path, sizeof(path), "%s/%s.log", LOG_DIR, tag);
        auto& self = inst();
        self.m_file = std::fopen(path, "w");
        if (self.m_file) {
            std::setvbuf(self.m_file, nullptr, _IOLBF, 0);
            std::fprintf(self.m_file, "=== %s log start ===\n", tag);
            std::fflush(self.m_file);
        }
    }

    static void close() {
        auto& self = inst();
        if (self.m_file) {
            std::fprintf(self.m_file, "=== log end ===\n");
            std::fclose(self.m_file);
            self.m_file = nullptr;
        }
    }

    static void log(const char* fmt, ...) {
        char buf[512];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        uint64_t ticks = svcGetSystemTick();
        uint64_t ms = ticks / 19200ULL;
        unsigned secs = (unsigned)(ms / 1000);
        unsigned frac = (unsigned)(ms % 1000);

        std::fprintf(stderr, "[%u.%03u] %s\n", secs, frac, buf);
        auto& self = inst();
        if (self.m_file) {
            std::fprintf(self.m_file, "[%u.%03u] %s\n", secs, frac, buf);
            std::fflush(self.m_file);
        }
    }

private:
    static FileLog& inst() { static FileLog s; return s; }
    FILE* m_file = nullptr;
};

}
