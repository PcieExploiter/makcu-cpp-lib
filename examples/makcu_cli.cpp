#include "makcu/Mouse.h"
#include "makcu/SerialTransport.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

namespace {

void printHelp() {
    std::cout << "\nMAKCU CLI\n";
    std::cout << "=========\n\n";
    std::cout << "Commands:\n";
    std::cout << "  help, h              Show help\n";
    std::cout << "  connect, c           Connect to device\n";
    std::cout << "  disconnect, d        Disconnect\n";
    std::cout << "  status, s            Connection status\n";
    std::cout << "  version, v           Firmware version\n";
    std::cout << "  send <cmd>           Send raw command\n";
    std::cout << "  move <x> <y>        Move mouse\n";
    std::cout << "  click <btn>          Click button\n";
    std::cout << "  press <btn>         Press button\n";
    std::cout << "  release <btn>        Release button\n";
    std::cout << "  scroll <delta>      Scroll wheel\n";
    std::cout << "  lock <target> <on/off>  Lock button/axis\n";
    std::cout << "  info                 Device info\n";
    std::cout << "  quit, q, exit        Exit\n";
    std::cout << "\n";
}

makcu::MouseButton parseButton(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower == "left" || lower == "l") return makcu::MouseButton::LEFT;
    if (lower == "right" || lower == "r") return makcu::MouseButton::RIGHT;
    if (lower == "middle" || lower == "m") return makcu::MouseButton::MIDDLE;
    if (lower == "mouse4" || lower == "m4" || lower == "4") return makcu::MouseButton::MOUSE4;
    if (lower == "mouse5" || lower == "m5" || lower == "5") return makcu::MouseButton::MOUSE5;
    
    throw std::invalid_argument("Invalid button: " + str);
}

std::vector<std::string> tokenize(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool handleCommand(const std::string& line, makcu::SerialTransport* transport, 
                  makcu::Mouse* mouse) {
    const auto tokens = tokenize(line);
    if (tokens.empty()) {
        return true;
    }
    
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    try {
        if (cmd == "help" || cmd == "h") {
            printHelp();
        }
        else if (cmd == "connect" || cmd == "c") {
            if (!transport) {
                std::cout << "Transport not initialized\n";
                return true;
            }
            if (transport->isConnected()) {
                std::cout << "Already connected to " << transport->getPort() << "\n";
            } else {
                std::cout << "Connecting...\n";
                transport->connect();
                if (transport->isConnected()) {
                    std::cout << "Connected to " << transport->getPort() 
                              << " at " << transport->getBaudrate() << " baud\n";
                } else {
                    std::cout << "Connection failed\n";
                }
            }
        }
        else if (cmd == "disconnect" || cmd == "d") {
            if (transport && transport->isConnected()) {
                transport->disconnect();
                std::cout << "Disconnected\n";
            } else {
                std::cout << "Not connected\n";
            }
        }
        else if (cmd == "status" || cmd == "s") {
            if (transport) {
                if (transport->isConnected()) {
                    std::cout << "Connected: " << transport->getPort() 
                              << " @ " << transport->getBaudrate() << " baud\n";
                } else {
                    std::cout << "Disconnected\n";
                }
            }
        }
        else if (cmd == "version" || cmd == "v") {
            if (mouse && transport && transport->isConnected()) {
                const auto version = mouse->getFirmwareVersion();
                std::cout << "Version: " << (version.empty() ? "N/A" : version) << "\n";
            } else {
                std::cout << "Not connected\n";
            }
        }
        else if (cmd == "send") {
            if (!transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            if (tokens.size() < 2) {
                std::cout << "Usage: send <command>\n";
                return true;
            }
            
            std::string command = line.substr(cmd.length());
            const size_t first = command.find_first_not_of(" \t");
            if (first != std::string::npos) {
                command = command.substr(first);
            }
            
            std::cout << "Sending: " << command << "\n";
            try {
                const auto response = transport->sendCommand(command, true, 0.5);
                if (!response.empty()) {
                    std::cout << "Response: " << response << "\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << "\n";
            }
        }
        else if (cmd == "move") {
            if (!mouse || !transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            if (tokens.size() < 3) {
                std::cout << "Usage: move <x> <y>\n";
                return true;
            }
            const int x = std::stoi(tokens[1]);
            const int y = std::stoi(tokens[2]);
            mouse->move(x, y);
        }
        else if (cmd == "click") {
            if (!mouse || !transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            if (tokens.size() < 2) {
                std::cout << "Usage: click <button>\n";
                return true;
            }
            mouse->click(parseButton(tokens[1]));
        }
        else if (cmd == "press") {
            if (!mouse || !transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            if (tokens.size() < 2) {
                std::cout << "Usage: press <button>\n";
                return true;
            }
            mouse->press(parseButton(tokens[1]));
        }
        else if (cmd == "release") {
            if (!mouse || !transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            if (tokens.size() < 2) {
                std::cout << "Usage: release <button>\n";
                return true;
            }
            mouse->release(parseButton(tokens[1]));
        }
        else if (cmd == "scroll") {
            if (!mouse || !transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            if (tokens.size() < 2) {
                std::cout << "Usage: scroll <delta>\n";
                return true;
            }
            const int delta = std::stoi(tokens[1]);
            mouse->scroll(delta);
        }
        else if (cmd == "lock") {
            if (!mouse || !transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            if (tokens.size() < 3) {
                std::cout << "Usage: lock <target> <on/off>\n";
                return true;
            }
            
            std::string target = tokens[1];
            std::transform(target.begin(), target.end(), target.begin(), 
                         [](unsigned char c) { return std::tolower(c); });
            std::string state = tokens[2];
            std::transform(state.begin(), state.end(), state.begin(), 
                         [](unsigned char c) { return std::tolower(c); });
            
            const bool lock = (state == "on" || state == "1" || state == "true");
            
            if (target == "x") {
                mouse->lockX(lock);
            } else if (target == "y") {
                mouse->lockY(lock);
            } else if (target == "left") {
                mouse->lockLeft(lock);
            } else if (target == "right") {
                mouse->lockRight(lock);
            } else if (target == "middle") {
                mouse->lockMiddle(lock);
            } else if (target == "side1" || target == "mouse4") {
                mouse->lockSide1(lock);
            } else if (target == "side2" || target == "mouse5") {
                mouse->lockSide2(lock);
            } else {
                std::cout << "Invalid target: " << tokens[1] << "\n";
                return true;
            }
            std::cout << target << " " << (lock ? "locked" : "unlocked") << "\n";
        }
        else if (cmd == "info") {
            if (!mouse || !transport || !transport->isConnected()) {
                std::cout << "Not connected\n";
                return true;
            }
            const auto info = mouse->getDeviceInfo();
            for (const auto& [key, value] : info) {
                std::cout << key << ": " << value << "\n";
            }
        }
        else if (cmd == "quit" || cmd == "q" || cmd == "exit") {
            if (transport && transport->isConnected()) {
                transport->disconnect();
            }
            return false;
        }
        else {
            std::cout << "Unknown command: " << cmd << "\n";
            std::cout << "Type 'help' for commands\n";
        }
    } catch (const makcu::MakcuError& e) {
        std::cout << "Error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    
    return true;
}

} // anonymous namespace

int main() {
    std::cout << "MAKCU CLI\n";
    std::cout << "Type 'help' for commands, 'quit' to exit\n\n";
    
    makcu::SerialTransport::Config config;
    config.debug = true;
    config.sendInit = true;
    config.autoReconnect = true;
    
    makcu::SerialTransport transport(config);
    makcu::Mouse mouse(transport);
    
    std::string line;
    while (true) {
        std::cout << "makcu> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        if (!handleCommand(line, &transport, &mouse)) {
            break;
        }
    }
    
    if (transport.isConnected()) {
        transport.disconnect();
    }
    
    return 0;
}
