#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

class DebugLog {
public:
    static constexpr int MAX_LINES = 30;
    static constexpr const char* LOG_DIR  = "sdmc:/config/SwitchU";
    static constexpr const char* LOG_FILE = "sdmc:/config/SwitchU/log.txt";

    static void openFileLog() {
        ::mkdir("sdmc:/config", 0755);
        ::mkdir(LOG_DIR, 0755);
        auto& self = instance();
        std::lock_guard<std::mutex> lk(self.m_mutex);
        self.m_file = std::fopen(LOG_FILE, "w");
        if (self.m_file) {
            std::setvbuf(self.m_file, nullptr, _IOLBF, 0);
            std::fprintf(self.m_file, "=== SwitchU log start ===\n");
            std::fflush(self.m_file);
        }
    }

    static void closeFileLog() {
        auto& self = instance();
        std::lock_guard<std::mutex> lk(self.m_mutex);
        if (self.m_file) {
            std::fprintf(self.m_file, "=== SwitchU log end ===\n");
            std::fclose(self.m_file);
            self.m_file = nullptr;
        }
    }

    static void log(const char* fmt, ...) {
        char buf[512];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        auto& self = instance();
        std::lock_guard<std::mutex> lk(self.m_mutex);

        self.m_lines.push_back(std::string(buf));
        if ((int)self.m_lines.size() > MAX_LINES)
            self.m_lines.erase(self.m_lines.begin());

        if (self.m_file) {
            std::fprintf(self.m_file, "%s\n", buf);
            std::fflush(self.m_file);
        }

        std::fprintf(stderr, "%s\n", buf);
    }

    static std::vector<std::string> lines() {
        auto& self = instance();
        std::lock_guard<std::mutex> lk(self.m_mutex);
        return self.m_lines;
    }

private:
    static DebugLog& instance() {
        static DebugLog s;
        return s;
    }
    std::mutex m_mutex;
    std::vector<std::string> m_lines;
    FILE* m_file = nullptr;
};
