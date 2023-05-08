#include <chrono>
#include <time.hpp>

#include <iostream>

namespace HomeAutomation {
namespace Library {
namespace Util {

static std::int64_t to_millis_since_start(TimeStamp const &ts) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             ts.time_since_epoch())
      .count();
}

void RegisterComponent(sol::state &lua) {
  lua["to_millis_since_start"] = &to_millis_since_start;
}

} // namespace Util
} // namespace Library
} // namespace HomeAutomation
