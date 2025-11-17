#include "makcu/Mouse.h"
#include "makcu/SerialTransport.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
        std::cout << "MAKCU C++ Library Example\n";
        std::cout << "=========================\n\n";
        
        makcu::SerialTransport::Config config;
        config.debug = true;
        
        makcu::SerialTransport transport(config);
        
        std::cout << "Connecting to MAKCU device...\n";
        transport.connect();
        
        if (!transport.isConnected()) {
            std::cerr << "Failed to connect\n";
            return 1;
        }
        
        std::cout << "Connected to " << transport.getPort() 
                  << " at " << transport.getBaudrate() << " baud\n";
        
        makcu::Mouse mouse(transport);
        
        std::cout << "\nGetting firmware version...\n";
        const auto version = mouse.getFirmwareVersion();
        std::cout << "Version: " << (version.empty() ? "N/A" : version) << "\n";
        
        std::cout << "\nTesting mouse movement...\n";
        mouse.move(10, 10);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        mouse.move(-10, -10);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Testing click...\n";
        mouse.click(makcu::MouseButton::LEFT);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Testing scroll...\n";
        mouse.scroll(3);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        mouse.scroll(-3);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "\nDone!\n";
        
        transport.disconnect();
        
    } catch (const makcu::MakcuError& e) {
        std::cerr << "MAKCU Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
