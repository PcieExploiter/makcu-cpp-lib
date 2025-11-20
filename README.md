# MAKCU C++ Library

A production-ready C++20 library for the MAKCU hardware mouse controller, providing high-performance mouse input with sub-millisecond latency.

**Version:** 1.0.1

## Features

- **High-Performance Mouse Control**: Direct hardware communication via serial port at 4Mbps
- **Low Latency**: ~0.04ms response time
- **Full Mouse Control**: Movement, clicks, scrolling, button locking
- **Auto-Detection**: Automatically finds MAKCU device on COM ports
- **Thread-Safe**: Safe for use in multi-threaded applications

## Requirements

- Windows 10/11
- Visual Studio 2019 or later (Community Edition supported)
- C++20 compatible compiler

The solution contains three projects:
- **makcu-cpp-lib**: Static library
- **example**: Simple example program
- **makcu_cli**: Interactive command-line interface for sending MAKCU commands

### Project Structure

```
makcu-cpp-lib/
├── include/
│   └── makcu/
│       ├── enums.h           # Mouse button enumerations
│       ├── errors.h           # Exception classes
│       ├── SerialTransport.h  # Serial communication layer
│       ├── Mouse.h            # Mouse control interface
│       ├── version.h          # Version information
│       └── makcu.h           # Main header (includes all)
├── src/
│   ├── SerialTransport.cpp
│   └── Mouse.cpp
├── examples/
│   ├── example.cpp            # Simple example
│   └── makcu_cli.cpp         # Interactive CLI
```

## Usage

### Quick Start

```cpp
#include "makcu/makcu.h"  // Includes all headers

// Or include individually:
// #include "makcu/SerialTransport.h"
// #include "makcu/Mouse.h"

int main() {
    makcu::SerialTransport::Config config;
    config.debug = false;  // Set to true for debug output
    
    makcu::SerialTransport transport(config);
    transport.connect();
    
    makcu::Mouse mouse(transport);
    
    mouse.move(10, 10);
    mouse.click(makcu::MouseButton::LEFT);
    mouse.scroll(3);
    
    transport.disconnect();
    return 0;
}
```

### Mouse movement Features

```cpp
// Smooth movement
mouse.moveSmooth(100, 100, 10);  // Move with 10 segments

// Bezier curve movement
mouse.moveBezier(100, 100, 20, 50, 50);  // Control points

// Absolute movement (moves to screen coordinates)
mouse.moveAbs({1920/2, 1080/2}, 5, 2);  // Center of 1920x1080 screen

// Button locking
mouse.lockX(true);   // Lock X axis
mouse.lockY(true);   // Lock Y axis
mouse.lockLeft(true); // Lock left button

// Get device info
std::string version = mouse.getFirmwareVersion();
auto info = mouse.getDeviceInfo();
```

## API Reference

### SerialTransport

- `void connect()` - Connect to MAKCU device
- `void disconnect()` - Disconnect from device
- `bool isConnected()` - Check connection status
- `void sendCommand(const std::string& command)` - Send command without response
- `std::string sendCommand(const std::string& command, bool expectResponse, double timeout)` - Send command and wait for response

### Mouse

- `void move(int x, int y)` - Relative mouse movement
- `void moveSmooth(int x, int y, int segments)` - Smooth movement
- `void moveBezier(int x, int y, int segments, int ctrlX, int ctrlY)` - Bezier curve movement
- `void moveAbs(std::pair<int, int> target, int speed, int waitMs)` - Absolute movement
- `void press(MouseButton button)` - Press button
- `void release(MouseButton button)` - Release button
- `void click(MouseButton button)` - Click button
- `void scroll(int delta)` - Scroll wheel
- `void lockX(bool lock)` / `void lockY(bool lock)` - Lock axes
- `void lockLeft(bool lock)` / `void lockRight(bool lock)` / etc. - Lock buttons
- `std::string getFirmwareVersion()` - Get firmware version

## Error Handling

All functions throw exceptions derived from `makcu::MakcuError`:

- `MakcuConnectionError` - Connection issues
- `MakcuCommandError` - Invalid commands
- `MakcuTimeoutError` - Command timeout
- `MakcuResponseError` - Invalid response

```cpp
try {
    transport.connect();
    mouse.move(10, 10);
} catch (const makcu::MakcuConnectionError& e) {
    std::cerr << "Connection failed: " << e.what() << std::endl;
} catch (const makcu::MakcuError& e) {
    std::cerr << "MAKCU error: " << e.what() << std::endl;
}
```

## Command Line Interface

The `makcu_cli` project provides an interactive command-line interface for sending MAKCU commands:

```bash
makcu_cli.exe
```

### CLI Commands

- `connect` / `c` - Connect to MAKCU device
- `disconnect` / `d` - Disconnect from device
- `status` / `s` - Show connection status
- `version` / `v` - Get firmware version
- `send <command>` - Send raw command (e.g., `send km.version()`)
- `move <x> <y>` - Move mouse (e.g., `move 10 20`)
- `click <button>` - Click button (left, right, middle, mouse4, mouse5)
- `press <button>` - Press button down
- `release <button>` - Release button
- `scroll <delta>` - Scroll wheel (e.g., `scroll 3` or `scroll -3`)
- `lock <target> <on/off>` - Lock button/axis (e.g., `lock x on`, `lock left off`)
- `info` - Get device information
- `help` / `h` - Show help message
- `quit` / `q` / `exit` - Exit program

### Example CLI Session

```
makcu> connect
Connected to COM4 at 4000000 baud

makcu> send km.version()
Sending: km.version()
Response: (version info)

makcu> move 50 -30
Moving mouse: (50, -30)

makcu> click left
Clicking left button

makcu> scroll 5
Scrolling: 5

makcu> quit
Exiting...
```

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Credits

Based on the Python implementation: https://github.com/SleepyTotem/makcu-py-lib







