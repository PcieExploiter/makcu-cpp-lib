#include "makcu/SerialTransport.h"
#include "makcu/errors.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <Windows.h>

namespace makcu {

using Config = SerialTransport::Config;
using ButtonCallback = SerialTransport::ButtonCallback;

namespace {
    constexpr unsigned char BAUD_CHANGE_CMD[] = {0xDE, 0xAD, 0x05, 0x00, 0xA5, 0x00, 0x09, 0x3D, 0x00};
    constexpr DWORD INITIAL_BAUD = 115200;
    constexpr DWORD TARGET_BAUD = 4000000;
    constexpr DWORD BAUD_CHANGE_DELAY_MS = 20;
    constexpr double DEFAULT_TIMEOUT = 0.1;
    constexpr int MAX_RECONNECT_ATTEMPTS = 3;
    constexpr double RECONNECT_DELAY = 0.1;
}

class SerialTransportImpl {
public:
    Config config_;
    HANDLE hSerial_ = INVALID_HANDLE_VALUE;
    std::string port_;
    DWORD currentBaud_ = 0;
    
    std::atomic<bool> connected_{false};
    std::atomic<bool> stopListener_{false};
    std::thread listenerThread_;
    std::mutex commandMutex_;
    
    ButtonCallback buttonCallback_;
    int lastButtonMask_ = 0;
    int buttonStates_ = 0;
    int reconnectAttempts_ = 0;

    explicit SerialTransportImpl(const Config& cfg) : config_(cfg) {}
    
    ~SerialTransportImpl() {
        disconnect();
    }

    void log(const std::string& msg, const std::string& level = "INFO") const {
        if (!config_.debug) return;
        
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm_buf{};
        localtime_s(&tm_buf, &time);
        
        char timeStr[64];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm_buf);
        
        std::cout << "[" << timeStr << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << level << "] " << msg << std::endl;
    }

    std::string findComPort() {
        log("Scanning for COM ports");
        
        if (config_.overridePort && !config_.fallbackComPort.empty()) {
            return config_.fallbackComPort;
        }
        
        std::vector<std::string> ports;
        HKEY hKey;
        
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD index = 0;
            char valueName[256];
            char data[256];
            DWORD valueNameSize, dataSize, type;
            
            while (true) {
                valueNameSize = sizeof(valueName);
                dataSize = sizeof(data);
                
                if (RegEnumValueA(hKey, index, valueName, &valueNameSize, NULL, 
                                  &type, reinterpret_cast<LPBYTE>(data), &dataSize) != ERROR_SUCCESS) {
                    break;
                }
                
                if (type == REG_SZ && dataSize > 0) {
                    ports.emplace_back(data);
                }
                
                ++index;
            }
            
            RegCloseKey(hKey);
        }
        
        if (!ports.empty()) {
            return ports.front();
        }
        
        if (!config_.fallbackComPort.empty()) {
            return config_.fallbackComPort;
        }
        
        return {};
    }

    bool changeBaudTo4M() {
        if (hSerial_ == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        DWORD bytesWritten = 0;
        if (!WriteFile(hSerial_, BAUD_CHANGE_CMD, sizeof(BAUD_CHANGE_CMD), 
                       &bytesWritten, nullptr)) {
            return false;
        }
        
        FlushFileBuffers(hSerial_);
        Sleep(BAUD_CHANGE_DELAY_MS);
        
        DCB dcb{};
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(hSerial_, &dcb)) {
            return false;
        }
        
        dcb.BaudRate = TARGET_BAUD;
        if (!SetCommState(hSerial_, &dcb)) {
            return false;
        }
        
        currentBaud_ = TARGET_BAUD;
        return true;
    }

    bool openSerialPort(const std::string& portName) {
        if (hSerial_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial_);
            hSerial_ = INVALID_HANDLE_VALUE;
        }
        
        const std::string portPath = "\\\\.\\" + portName;
        hSerial_ = CreateFileA(portPath.c_str(), GENERIC_READ | GENERIC_WRITE,
                               0, nullptr, OPEN_EXISTING, 0, nullptr);
        
        if (hSerial_ == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        DCB dcb{};
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(hSerial_, &dcb)) {
            CloseHandle(hSerial_);
            hSerial_ = INVALID_HANDLE_VALUE;
            return false;
        }
        
        dcb.BaudRate = INITIAL_BAUD;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity = NOPARITY;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        
        if (!SetCommState(hSerial_, &dcb)) {
            CloseHandle(hSerial_);
            hSerial_ = INVALID_HANDLE_VALUE;
            return false;
        }
        
        COMMTIMEOUTS timeouts{};
        timeouts.ReadIntervalTimeout = 0;
        timeouts.ReadTotalTimeoutConstant = 1;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 1; // Reduced from 10ms to 1ms for lower latency
        timeouts.WriteTotalTimeoutMultiplier = 0;
        
        if (!SetCommTimeouts(hSerial_, &timeouts)) {
            CloseHandle(hSerial_);
            hSerial_ = INVALID_HANDLE_VALUE;
            return false;
        }
        
        PurgeComm(hSerial_, PURGE_RXCLEAR | PURGE_TXCLEAR);
        
        if (!changeBaudTo4M()) {
            CloseHandle(hSerial_);
            hSerial_ = INVALID_HANDLE_VALUE;
            return false;
        }
        
        port_ = portName;
        return true;
    }

    void closeSerialPort() {
        if (hSerial_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial_);
            hSerial_ = INVALID_HANDLE_VALUE;
        }
    }

    void listenerThread() {
        char buffer[4096];
        
        while (!stopListener_ && connected_) {
            DWORD bytesRead = 0;
            if (!ReadFile(hSerial_, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
                if (GetLastError() != ERROR_IO_PENDING) {
                    break;
                }
                Sleep(1);
                continue;
            }
            
            if (bytesRead == 0) {
                Sleep(1);
                continue;
            }
            
            buffer[bytesRead] = '\0';
            
            if (config_.debug) {
                std::string received(buffer, bytesRead);
                log("RX: " + received, "DEBUG");
            }
        }
    }

    void connect() {
        if (connected_) {
            return;
        }
        
        const std::string portToUse = findComPort();
        if (portToUse.empty()) {
            throw MakcuConnectionError("MAKCU device not found");
        }
        
        if (!openSerialPort(portToUse)) {
            throw MakcuConnectionError("Failed to open serial port: " + portToUse);
        }
        
        connected_ = true;
        reconnectAttempts_ = 0;
        
        if (config_.sendInit) {
            const std::string initCmd = "km.buttons(1)\r\n";
            DWORD bytesWritten = 0;
            WriteFile(hSerial_, initCmd.c_str(), 
                     static_cast<DWORD>(initCmd.length()), &bytesWritten, nullptr);
            FlushFileBuffers(hSerial_);
        }
        
        stopListener_ = false;
        listenerThread_ = std::thread(&SerialTransportImpl::listenerThread, this);
    }

    void disconnect() {
        connected_ = false;
        stopListener_ = true;
        
        if (listenerThread_.joinable()) {
            listenerThread_.join();
        }
        
        closeSerialPort();
    }

    std::string sendCommand(const std::string& cmd, bool expectResponse = false, 
                           double timeoutSeconds = DEFAULT_TIMEOUT) {
        if (!connected_ || hSerial_ == INVALID_HANDLE_VALUE) {
            throw MakcuConnectionError("Not connected");
        }
        
        const std::string formattedCmd = cmd + "\r\n";
        DWORD bytesWritten = 0;
        
        if (!WriteFile(hSerial_, formattedCmd.c_str(), 
                      static_cast<DWORD>(formattedCmd.length()), &bytesWritten, nullptr)) {
            throw MakcuConnectionError("Failed to write to serial port");
        }
        
        // Only flush for critical commands, not for rapid mouse movements
        // This significantly reduces latency for high-frequency move commands
        if (expectResponse || cmd.find("km.move") == std::string::npos) {
            FlushFileBuffers(hSerial_);
        }
        
        if (expectResponse) {
            Sleep(static_cast<DWORD>(timeoutSeconds * 1000));
            return {};
        }
        
        return cmd;
    }
};

SerialTransport::SerialTransport(const Config& config) 
    : pImpl(std::make_unique<SerialTransportImpl>(config)) {
}

SerialTransport::~SerialTransport() = default;

void SerialTransport::connect() {
    pImpl->connect();
}

void SerialTransport::disconnect() {
    pImpl->disconnect();
}

bool SerialTransport::isConnected() const {
    return pImpl->connected_;
}

void SerialTransport::sendCommand(const std::string& command) {
    pImpl->sendCommand(command, false);
}

std::string SerialTransport::sendCommand(const std::string& command, bool expectResponse, 
                                         double timeoutSeconds) {
    return pImpl->sendCommand(command, expectResponse, timeoutSeconds);
}

void SerialTransport::setButtonCallback(ButtonCallback callback) {
    pImpl->buttonCallback_ = callback;
}

void SerialTransport::enableButtonMonitoring(bool enable) {
    sendCommand(enable ? "km.buttons(1)" : "km.buttons(0)");
}

std::string SerialTransport::getPort() const {
    return pImpl->port_;
}

int SerialTransport::getBaudrate() const {
    return static_cast<int>(pImpl->currentBaud_);
}

} // namespace makcu
