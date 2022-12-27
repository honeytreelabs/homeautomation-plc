#include <components_registry_lua.hpp>
#include <light.hpp>
#include <trigger.hpp>

namespace HomeAutomation {
namespace Components {

void HomeAutomation::Components::LuaComponentsRegistry::RegisterComponents(
    sol::state &lua) {
  Light::RegisterComponent(lua);
  R_TRIG::RegisterComponent(lua);
  F_TRIG::RegisterComponent(lua);
}

} // namespace Components
} // namespace HomeAutomation
