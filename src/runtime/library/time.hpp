#pragma once

#include <common.hpp>

#include <sol/sol.hpp>

#include <cstdint>

namespace HomeAutomation {
namespace Library {
namespace Util {

void RegisterComponent(sol::state &lua);

} // namespace Util
} // namespace Library
} // namespace HomeAutomation
