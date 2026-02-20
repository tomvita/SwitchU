#pragma once
#include "../core/Types.hpp"
#include <string>
#include <vector>

namespace ui {

struct AppEntry {
    std::string id;
    std::string title;
    uint64_t    titleId = 0;
    int         iconTexIndex = -1;
    Color       tint = Color::white();
};

class GridModel {
public:
    void clear() { m_entries.clear(); }
    void addEntry(AppEntry e) { m_entries.push_back(std::move(e)); }
    int  count() const { return (int)m_entries.size(); }
    const AppEntry& at(int i) const { return m_entries[i]; }
    const std::vector<AppEntry>& entries() const { return m_entries; }

private:
    std::vector<AppEntry> m_entries;
};

} // namespace ui
