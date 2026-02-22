#pragma once
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>

// Simple on-screen debug log for qlaunch debugging.
// Call DebugLog::log("message") anywhere, then DebugLog::render() each frame.
class DebugLog {
public:
    static constexpr int MAX_LINES = 30;

    static void log(const char* fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        auto& lines = instance().m_lines;
        lines.push_back(std::string(buf));
        if ((int)lines.size() > MAX_LINES)
            lines.erase(lines.begin());
    }

    static std::vector<std::string>& lines() {
        return instance().m_lines;
    }

private:
    static DebugLog& instance() {
        static DebugLog s;
        return s;
    }
    std::vector<std::string> m_lines;
};
