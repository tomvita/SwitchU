#pragma once

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/time.h>
#include <system_error>
#include <vector>

namespace switchu::log_detail {

inline void ensure_log_dir(const char* log_dir) {
    std::error_code ec;
    std::filesystem::create_directory("sdmc:/config", ec);
    ec.clear();
    std::filesystem::create_directory(log_dir, ec);
}

inline bool read_local_time(std::tm& local_time, long& milliseconds, long& tenths) {
    timeval now{};
    if (::gettimeofday(&now, nullptr) != 0) {
        std::time_t fallback = std::time(nullptr);
        if (fallback == static_cast<std::time_t>(-1) || !::localtime_r(&fallback, &local_time))
            return false;

        milliseconds = 0;
        tenths = 0;
        return true;
    }

    std::time_t seconds = static_cast<std::time_t>(now.tv_sec);
    if (!::localtime_r(&seconds, &local_time))
        return false;

    milliseconds = now.tv_usec / 1000L;
    tenths = now.tv_usec / 100000L;
    return true;
}

inline void format_archive_timestamp(char* buffer, size_t size) {
    std::tm local_time{};
    long milliseconds = 0;
    long tenths = 0;
    if (!read_local_time(local_time, milliseconds, tenths)) {
        std::snprintf(buffer, size, "0000-00-00_00-00-00-000");
        return;
    }

    std::snprintf(buffer, size,
                  "%04d-%02d-%02d_%02d-%02d-%02d-%03ld",
                  local_time.tm_year + 1900,
                  local_time.tm_mon + 1,
                  local_time.tm_mday,
                  local_time.tm_hour,
                  local_time.tm_min,
                  local_time.tm_sec,
                  milliseconds);
}

inline void format_line_timestamp(char* buffer, size_t size) {
    std::tm local_time{};
    long milliseconds = 0;
    long tenths = 0;
    if (!read_local_time(local_time, milliseconds, tenths)) {
        std::snprintf(buffer, size, "0000-00-00 00:00:00.0");
        return;
    }

    std::snprintf(buffer, size,
                  "%04d-%02d-%02d %02d:%02d:%02d.%01ld",
                  local_time.tm_year + 1900,
                  local_time.tm_mon + 1,
                  local_time.tm_mday,
                  local_time.tm_hour,
                  local_time.tm_min,
                  local_time.tm_sec,
                  tenths);
}

inline void build_current_log_path(char* path, size_t size, const char* log_dir, const char* base_name, const char* extension) {
    std::snprintf(path, size, "%s/%s%s", log_dir, base_name, extension);
}

inline void build_archived_log_path(char* path, size_t size, const char* log_dir, const char* base_name, const char* extension) {
    char timestamp[32];
    format_archive_timestamp(timestamp, sizeof(timestamp));
    std::snprintf(path, size, "%s/%s-%s%s", log_dir, base_name, timestamp, extension);
}

inline bool has_archived_log_name(const char* file_name, const char* base_name, const char* extension) {
    char prefix[128];
    std::snprintf(prefix, sizeof(prefix), "%s-", base_name);

    const size_t prefix_len = std::strlen(prefix);
    const size_t file_len = std::strlen(file_name);
    const size_t extension_len = std::strlen(extension);

    return file_len > prefix_len + 4
        && std::strncmp(file_name, prefix, prefix_len) == 0
        && std::strcmp(file_name + file_len - extension_len, extension) == 0;
}

inline void prune_archived_logs(const char* log_dir, const char* base_name, const char* extension, size_t keep_count) {
    struct ArchivedLog {
        std::string name;
        std::time_t mtime;
    };
    std::vector<ArchivedLog> matches;
    std::error_code ec;
    char path[256];
    for (const auto& entry : std::filesystem::directory_iterator(log_dir, ec)) {
        if (ec)
            break;

        const std::string file_name = entry.path().filename().string();
        if (!has_archived_log_name(file_name.c_str(), base_name, extension))
            continue;
        std::snprintf(path, sizeof(path), "%s/%s", log_dir, file_name.c_str());
        struct stat st{};
        matches.push_back({file_name, ::stat(path, &st) == 0 ? st.st_mtime : 0});
    }

    if (matches.size() <= keep_count)
        return;

    // Archive names carry the boot-time clock, which is still 1970 when the
    // daemon starts, so name order says nothing about age. The SD card's
    // modification time does.
    std::sort(matches.begin(), matches.end(), [](const ArchivedLog& a, const ArchivedLog& b) {
        return a.mtime != b.mtime ? a.mtime < b.mtime : a.name < b.name;
    });

    const size_t delete_count = matches.size() - keep_count;
    for (size_t i = 0; i < delete_count; ++i) {
        std::snprintf(path, sizeof(path), "%s/%s", log_dir, matches[i].name.c_str());
        ec.clear();
        std::filesystem::remove(path, ec);
    }
}

// Batches log lines before touching the SD card.
//
// Every line used to force an fflush. Menu startup emits ~50 lines back to
// back, so that was ~50 blocking SD writes on the critical path before the
// first frame, plus a stall whenever anything logged mid-frame. Lines are now
// accumulated and written once the buffer fills or the sink is closed.
class buffered_sink {
public:
    static constexpr size_t flush_threshold_bytes = 4096;

    bool is_open() const { return m_file.is_open(); }

    void open(const char* path, bool truncate) {
        m_file.open(path, truncate ? std::ios::out | std::ios::trunc
                                   : std::ios::out | std::ios::app);
    }

    void append(std::string_view line) {
        if (!m_file.is_open())
            return;
        if (m_pending.empty())
            m_pending_since = std::time(nullptr);
        m_pending.append(line);
        m_pending.push_back('\n');
        if (m_pending.size() >= flush_threshold_bytes)
            flush();
    }

    // For long-lived processes that never close the sink (the daemon), so a
    // handful of buffered lines don't sit unwritten indefinitely.
    void flush_if_stale(std::time_t max_age_seconds) {
        if (m_pending.empty())
            return;
        if (std::time(nullptr) - m_pending_since >= max_age_seconds)
            flush();
    }

    void flush() {
        if (!m_file.is_open() || m_pending.empty())
            return;
        m_file.write(m_pending.data(), static_cast<std::streamsize>(m_pending.size()));
        m_file.flush();
        m_pending.clear();
    }

    void close() {
        if (!m_file.is_open())
            return;
        flush();
        m_file.close();
    }

private:
    std::ofstream m_file;
    std::string m_pending;
    std::time_t m_pending_since = 0;
};

inline bool rotate_current_log(const char* log_dir, const char* base_name, const char* extension, size_t keep_count) {
    char current_path[256];
    build_current_log_path(current_path, sizeof(current_path), log_dir, base_name, extension);

    bool can_truncate = true;
    std::error_code ec;
    if (std::filesystem::exists(current_path, ec)) {
        char archived_path[256];
        build_archived_log_path(archived_path, sizeof(archived_path), log_dir, base_name, extension);
        // The clock isn't set yet when the daemon starts, so every boot builds
        // nearly the same name; without this the previous boot's log is lost.
        if (std::filesystem::exists(archived_path, ec)) {
            char unique_path[256];
            for (int attempt = 2; attempt <= 9; ++attempt) {
                std::snprintf(unique_path, sizeof(unique_path), "%.*s-%d%s",
                              static_cast<int>(std::strlen(archived_path) - std::strlen(extension)),
                              archived_path, attempt, extension);
                ec.clear();
                if (!std::filesystem::exists(unique_path, ec)) {
                    std::snprintf(archived_path, sizeof(archived_path), "%s", unique_path);
                    break;
                }
            }
        }
        ec.clear();
        std::filesystem::rename(current_path, archived_path, ec);
        can_truncate = !ec;
    }

    prune_archived_logs(log_dir, base_name, extension, keep_count);
    return can_truncate;
}

} // namespace switchu::log_detail
