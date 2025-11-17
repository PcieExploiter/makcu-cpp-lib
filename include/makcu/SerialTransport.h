#pragma once
#include "enums.h"
#include "errors.h"
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <Windows.h>

namespace makcu {

class SerialTransportImpl;

class SerialTransport {
public:
    struct Config {
        std::string fallbackComPort;
        bool debug = false;
        bool sendInit = true;
        bool autoReconnect = true;
        bool overridePort = false;
    };

    using ButtonCallback = std::function<void(MouseButton, bool)>;

    explicit SerialTransport(const Config& config = Config());
    ~SerialTransport();

    SerialTransport(const SerialTransport&) = delete;
    SerialTransport& operator=(const SerialTransport&) = delete;
    SerialTransport(SerialTransport&&) = delete;
    SerialTransport& operator=(SerialTransport&&) = delete;

    void connect();
    void disconnect();
    [[nodiscard]] bool isConnected() const;

    void sendCommand(const std::string& command);
    std::string sendCommand(const std::string& command, bool expectResponse, 
                           double timeoutSeconds = 0.1);

    void setButtonCallback(ButtonCallback callback);
    void enableButtonMonitoring(bool enable = true);
    
    [[nodiscard]] std::string getPort() const;
    [[nodiscard]] int getBaudrate() const;

private:
    std::unique_ptr<SerialTransportImpl> pImpl;
};

} // namespace makcu
