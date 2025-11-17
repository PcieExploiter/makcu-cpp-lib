#pragma once
#include "enums.h"
#include "errors.h"
#include "SerialTransport.h"
#include <string>
#include <map>
#include <utility>

namespace makcu {

class Mouse {
public:
    explicit Mouse(SerialTransport& transport);
    ~Mouse() = default;

    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;
    Mouse(Mouse&&) = delete;
    Mouse& operator=(Mouse&&) = delete;

    void press(MouseButton button);
    void release(MouseButton button);
    void click(MouseButton button);

    void move(int x, int y);
    void moveSmooth(int x, int y, int segments);
    void moveBezier(int x, int y, int segments, int ctrlX, int ctrlY);
    void moveAbs(std::pair<int, int> target, int speed = 1, int waitMs = 2);

    void scroll(int delta);

    void lockLeft(bool lock);
    void lockRight(bool lock);
    void lockMiddle(bool lock);
    void lockSide1(bool lock);
    void lockSide2(bool lock);
    void lockX(bool lock);
    void lockY(bool lock);

    [[nodiscard]] bool isLocked(MouseButton button);
    [[nodiscard]] bool isLocked(const std::string& axisName);
    [[nodiscard]] std::map<std::string, bool> getAllLockStates();

    [[nodiscard]] std::string getFirmwareVersion();
    [[nodiscard]] std::map<std::string, std::string> getDeviceInfo();

    void spoofSerial(const std::string& serial);
    void resetSerial();

private:
    SerialTransport& transport_;
    int lockStatesCache_;
    bool cacheValid_;

    [[nodiscard]] std::string getButtonCommand(MouseButton button) const;
    [[nodiscard]] std::string getPressCommand(MouseButton button) const;
    [[nodiscard]] std::string getReleaseCommand(MouseButton button) const;
    void updateLockCache();
};

} // namespace makcu
