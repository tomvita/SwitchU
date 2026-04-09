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
        auto& self = inst();
        std::snprintf(self.m_path, sizeof(self.m_path), "%s/%s.log", LOG_DIR, tag);
        FILE* f = std::fopen(self.m_path, "a");
        if (f) {
            std::fprintf(f, "\n=== %s log start ===\n", tag);
            std::fclose(f);
        }
    }

    static void close() {
        auto& self = inst();
        FILE* f = std::fopen(self.m_path, "a");
        if (f) {
            std::fprintf(f, "=== log end ===\n");
            std::fclose(f);
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
        FILE* f = std::fopen(self.m_path, "a");
        if (f) {
            std::fprintf(f, "[%u.%03u] %s\n", secs, frac, buf);
            std::fclose(f);
        }
    }

private:
    static FileLog& inst() { static FileLog s; return s; }
    char m_path[128] = {};
};

}
