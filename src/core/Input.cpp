#include "Input.hpp"
#include <cmath>

namespace ui {

void Input::initialize() {
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&m_pad);
    hidInitializeTouchScreen();
}

void Input::update() {
    padUpdate(&m_pad);
    m_kDown = padGetButtonsDown(&m_pad);
    m_kUp   = padGetButtonsUp(&m_pad);
    m_kHeld = padGetButtons(&m_pad);

    // Analog sticks
    HidAnalogStickState ls = padGetStickPos(&m_pad, 0);
    HidAnalogStickState rs = padGetStickPos(&m_pad, 1);
    m_lx = ls.x / 32767.f;
    m_ly = ls.y / 32767.f;
    m_rx = rs.x / 32767.f;
    m_ry = rs.y / 32767.f;

    // Touch
    HidTouchScreenState tState = {};
    m_wasTouching = m_touching;
    if (hidGetTouchScreenStates(&tState, 1) && tState.count > 0) {
        m_touching = true;
        m_touchX = tState.touches[0].x;
        m_touchY = tState.touches[0].y;
    } else {
        m_touching = false;
    }

    m_touchDown = (m_touching && !m_wasTouching);
    m_touchUp   = (!m_touching && m_wasTouching);

    if (m_touchDown) {
        m_touchStartX = m_touchX;
        m_touchStartY = m_touchY;
        m_touchStartTick = armGetSystemTick();
    }
}

float Input::touchDuration() const {
    uint64_t now = armGetSystemTick();
    return static_cast<float>(now - m_touchStartTick) / static_cast<float>(armGetSystemTickFreq());
}

bool Input::isDown(Button b) const { return m_kDown & static_cast<uint64_t>(b); }
bool Input::isUp(Button b)   const { return m_kUp   & static_cast<uint64_t>(b); }
bool Input::isHeld(Button b) const { return m_kHeld & static_cast<uint64_t>(b); }

} // namespace ui
