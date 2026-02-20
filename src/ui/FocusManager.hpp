#pragma once
#include "Focusable.hpp"
#include <vector>
#include <functional>

namespace ui {

class FocusManager {
public:
    using FocusChangedCb = std::function<void(IFocusable*, IFocusable*)>;

    void setGrid(std::vector<IFocusable*> items, int cols);
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    void setFocus(int index);
    int  focusIndex()   const { return m_index; }
    IFocusable* current() const;

    void onFocusChanged(FocusChangedCb cb) { m_cb = cb; }

    int  columns() const { return m_cols; }
    int  rows()    const;
    int  count()   const { return (int)m_items.size(); }

private:
    void changeFocus(int newIdx);
    std::vector<IFocusable*> m_items;
    int m_cols  = 1;
    int m_index = 0;
    FocusChangedCb m_cb;
};

} // namespace ui
