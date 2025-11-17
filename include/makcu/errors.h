#pragma once
#include <stdexcept>
#include <string>

namespace makcu {

class MakcuError : public std::runtime_error {
public:
    explicit MakcuError(const std::string& message) : std::runtime_error(message) {}
};

class MakcuConnectionError : public MakcuError {
public:
    explicit MakcuConnectionError(const std::string& message) : MakcuError(message) {}
};

class MakcuCommandError : public MakcuError {
public:
    explicit MakcuCommandError(const std::string& message) : MakcuError(message) {}
};

class MakcuTimeoutError : public MakcuError {
public:
    explicit MakcuTimeoutError(const std::string& message) : MakcuError(message) {}
};

class MakcuResponseError : public MakcuError {
public:
    explicit MakcuResponseError(const std::string& message) : MakcuError(message) {}
};

} // namespace makcu

