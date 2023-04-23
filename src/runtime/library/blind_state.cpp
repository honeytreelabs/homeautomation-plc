#include <blind_state.hpp>

namespace HomeAutomation {
namespace Library {

using namespace std::chrono_literals;

BlindConfig BlindConfigFromMillis(std::uint32_t periodIdle,
                                  std::uint32_t periodUp,
                                  std::uint32_t periodDown) {
  return BlindConfig{periodIdle * 1ms, periodUp * 1ms, periodDown * 1ms};
}

} // namespace Library
} // namespace HomeAutomation
