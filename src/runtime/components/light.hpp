#pragma once

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

#include <string>

namespace HomeAutomation {
namespace Components {

class Light {
public:
  Light(std::string name) : name{name}, state(false) {
    spdlog::info("Light {}: {}", name, state ? "on" : "off");
  }
  Light() : Light("") {}

  bool toggle() {
    state = !state;
    spdlog::info("Light {}: {}", name, state ? "on" : "off");
    return state;
  }
  bool getState() const { return state; }

  static void RegisterComponent(sol::state &lua) {
    sol::usertype<Light> light = lua.new_usertype<Light>(
        "Light", sol::constructors<Light(), Light(std::string)>());
    light["toggle"] = &Light::toggle;
    light["getState"] = &Light::getState;
  }

private:
  std::string name;
  bool state;
};
} // namespace Components
} // namespace HomeAutomation
