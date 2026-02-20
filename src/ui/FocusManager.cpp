// ui/FocusManager.cpp
#include "FocusManager.hpp"

namespace ui {

void FocusManager::setGrid(std::vector<IFocusable*> items, int cols) {
    m_items = std::move(items);
    m_cols  = cols;
    m_index = 0;
    if (!m_items.empty()) m_items[0]->onFocusGained();
}

int FocusManager::rows() const {
    if (m_cols == 0) return 0;
    return ((int)m_items.size() + m_cols - 1) / m_cols;
}

IFocusable* FocusManager::current() const {
    if (m_index < 0 || m_index >= (int)m_items.size()) return nullptr;
    return m_items[m_index];
}

void FocusManager::changeFocus(int newIdx) {
    if (newIdx < 0 || newIdx >= (int)m_items.size()) return;
    if (newIdx == m_index) return;
    auto* old = current();
    if (old) old->onFocusLost();
    m_index = newIdx;
    auto* cur = current();
    if (cur) cur->onFocusGained();
    if (m_cb) m_cb(old, cur);
}

void FocusManager::setFocus(int index) { changeFocus(index); }

void FocusManager::moveLeft() {
    int col = m_index % m_cols;
    if (col > 0) changeFocus(m_index - 1);
}

void FocusManager::moveRight() {
    int col = m_index % m_cols;
    if (col < m_cols - 1 && m_index + 1 < (int)m_items.size())
        changeFocus(m_index + 1);
}

void FocusManager::moveUp() {
    int newIdx = m_index - m_cols;
    if (newIdx >= 0) changeFocus(newIdx);
}

void FocusManager::moveDown() {
    int newIdx = m_index + m_cols;
    if (newIdx < (int)m_items.size()) changeFocus(newIdx);
}

} // namespace ui
