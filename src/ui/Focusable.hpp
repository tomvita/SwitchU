#pragma once
#include "../core/Types.hpp"
#include <string>

namespace ui {

class IFocusable {
public:
    virtual ~IFocusable() = default;
    virtual void onFocusGained() {}
    virtual void onFocusLost() {}
    virtual Rect getFocusRect() const = 0;
    virtual std::string getFocusId() const { return ""; }
    virtual bool isFocusable() const { return true; }
};

} // namespace ui
