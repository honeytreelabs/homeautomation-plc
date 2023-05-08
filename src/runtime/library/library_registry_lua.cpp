#include <blind.hpp>
#include <library_registry_lua.hpp>
#include <light.hpp>
#include <time.hpp>
#include <trigger.hpp>

namespace HomeAutomation {
namespace Library {

void HomeAutomation::Library::LuaLibraryRegistry::RegisterComponents(
    sol::state &lua) {
  Light::RegisterComponent(lua);
  R_TRIG::RegisterComponent(lua);
  F_TRIG::RegisterComponent(lua);
  Blind::RegisterComponent(lua);
  Util::RegisterComponent(lua);
}

} // namespace Library
} // namespace HomeAutomation
