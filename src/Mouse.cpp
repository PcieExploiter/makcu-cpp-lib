#include "makcu/Mouse.h"
#include "makcu/errors.h"
#include <sstream>
#include <map>
#include <algorithm>
#include <cmath>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace makcu {

namespace {
    const int SPEED_MIN = 1;
    const int SPEED_MAX = 14;
}

Mouse::Mouse(SerialTransport& transport) 
    : transport_(transport), lockStatesCache_(0), cacheValid_(false) {
}

std::string Mouse::getButtonCommand(MouseButton button) const {
    switch (button) {
        case MouseButton::LEFT:   return "left";
        case MouseButton::RIGHT:  return "right";
        case MouseButton::MIDDLE: return "middle";
        case MouseButton::MOUSE4: return "ms1";
        case MouseButton::MOUSE5: return "ms2";
        default:
            throw MakcuCommandError("Unsupported button");
    }
}

std::string Mouse::getPressCommand(MouseButton button) const {
    return "km." + getButtonCommand(button) + "(1)";
}

std::string Mouse::getReleaseCommand(MouseButton button) const {
    return "km." + getButtonCommand(button) + "(0)";
}

void Mouse::press(MouseButton button) {
    transport_.sendCommand(getPressCommand(button));
}

void Mouse::release(MouseButton button) {
    transport_.sendCommand(getReleaseCommand(button));
}

void Mouse::click(MouseButton button) {
    press(button);
    release(button);
}

void Mouse::move(int x, int y) {
    std::ostringstream cmd;
    cmd << "km.move(" << x << "," << y << ")";
    transport_.sendCommand(cmd.str());
}

void Mouse::moveSmooth(int x, int y, int segments) {
    std::ostringstream cmd;
    cmd << "km.move(" << x << "," << y << "," << segments << ")";
    transport_.sendCommand(cmd.str());
}

void Mouse::moveBezier(int x, int y, int segments, int ctrlX, int ctrlY) {
    std::ostringstream cmd;
    cmd << "km.move(" << x << "," << y << "," << segments 
        << "," << ctrlX << "," << ctrlY << ")";
    transport_.sendCommand(cmd.str());
}

void Mouse::moveAbs(std::pair<int, int> target, int speed, int waitMs) {
    const UINT SPI_GETMOUSESPEED_VALUE = 0x0070;
    UINT mouseSpeed = 10;
    SystemParametersInfoA(SPI_GETMOUSESPEED_VALUE, 0, &mouseSpeed, 0);
    const float multiplier = static_cast<float>(mouseSpeed) / 10.0f;
    
    const int clampedSpeed = std::clamp(speed, SPEED_MIN, SPEED_MAX);
    const int endX = target.first;
    const int endY = target.second;
    
    POINT pt;
    while (true) {
        GetCursorPos(&pt);
        const int dx = endX - pt.x;
        const int dy = endY - pt.y;
        
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            break;
        }
        
        const int moveX = std::clamp(static_cast<int>(dx / multiplier), 
                                     -clampedSpeed, clampedSpeed);
        const int moveY = std::clamp(static_cast<int>(dy / multiplier), 
                                     -clampedSpeed, clampedSpeed);
        
        move(moveX, moveY);
        Sleep(waitMs);
    }
}

void Mouse::scroll(int delta) {
    std::ostringstream cmd;
    cmd << "km.wheel(" << delta << ")";
    transport_.sendCommand(cmd.str());
}

void Mouse::lockLeft(bool lock) {
    transport_.sendCommand(lock ? "km.lock_ml(1)" : "km.lock_ml(0)");
    if (lock) {
        lockStatesCache_ |= (1 << 0);
    } else {
        lockStatesCache_ &= ~(1 << 0);
    }
    cacheValid_ = true;
}

void Mouse::lockRight(bool lock) {
    transport_.sendCommand(lock ? "km.lock_mr(1)" : "km.lock_mr(0)");
    if (lock) {
        lockStatesCache_ |= (1 << 1);
    } else {
        lockStatesCache_ &= ~(1 << 1);
    }
    cacheValid_ = true;
}

void Mouse::lockMiddle(bool lock) {
    transport_.sendCommand(lock ? "km.lock_mm(1)" : "km.lock_mm(0)");
    if (lock) {
        lockStatesCache_ |= (1 << 2);
    } else {
        lockStatesCache_ &= ~(1 << 2);
    }
    cacheValid_ = true;
}

void Mouse::lockSide1(bool lock) {
    transport_.sendCommand(lock ? "km.lock_ms1(1)" : "km.lock_ms1(0)");
    if (lock) {
        lockStatesCache_ |= (1 << 3);
    } else {
        lockStatesCache_ &= ~(1 << 3);
    }
    cacheValid_ = true;
}

void Mouse::lockSide2(bool lock) {
    transport_.sendCommand(lock ? "km.lock_ms2(1)" : "km.lock_ms2(0)");
    if (lock) {
        lockStatesCache_ |= (1 << 4);
    } else {
        lockStatesCache_ &= ~(1 << 4);
    }
    cacheValid_ = true;
}

void Mouse::lockX(bool lock) {
    transport_.sendCommand(lock ? "km.lock_mx(1)" : "km.lock_mx(0)");
    if (lock) {
        lockStatesCache_ |= (1 << 5);
    } else {
        lockStatesCache_ &= ~(1 << 5);
    }
    cacheValid_ = true;
}

void Mouse::lockY(bool lock) {
    transport_.sendCommand(lock ? "km.lock_my(1)" : "km.lock_my(0)");
    if (lock) {
        lockStatesCache_ |= (1 << 6);
    } else {
        lockStatesCache_ &= ~(1 << 6);
    }
    cacheValid_ = true;
}

bool Mouse::isLocked(MouseButton button) {
    return false;
}

bool Mouse::isLocked(const std::string& axisName) {
    return false;
}

std::map<std::string, bool> Mouse::getAllLockStates() {
    return {};
}

std::string Mouse::getFirmwareVersion() {
    return transport_.sendCommand("km.version()", true, 0.1);
}

std::map<std::string, std::string> Mouse::getDeviceInfo() {
    std::map<std::string, std::string> info;
    info["port"] = transport_.getPort();
    info["isConnected"] = transport_.isConnected() ? "true" : "false";
    return info;
}

void Mouse::spoofSerial(const std::string& serial) {
    std::ostringstream cmd;
    cmd << "km.serial('" << serial << "')";
    transport_.sendCommand(cmd.str());
}

void Mouse::resetSerial() {
    transport_.sendCommand("km.serial(0)");
}

void Mouse::updateLockCache() {
    cacheValid_ = true;
}

} // namespace makcu
