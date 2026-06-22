#pragma once

#include <functional>
#include <map>
#include <string>

namespace mpa {

struct ControllerState;
struct ExecutionContext;

struct ActionCall {
    std::string capability;
    std::string action;
    std::map<std::string, std::string> arguments;
};

using ActionHandler =
    std::function<void(const ActionCall&, ControllerState&, ExecutionContext&)>;

}  // namespace mpa
