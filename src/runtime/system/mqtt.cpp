#include <mqtt.hpp>

#include <optional>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace std::chrono_literals;

namespace HomeAutomation {
namespace IO {
namespace MQTT {

void Client::send(Topic const &topic, char const *payload) {
  auto length = std::strlen(payload);
  Payload payload_fwd;
  payload_fwd.reserve(length);
  for (std::size_t i = 0; i < length; i++) {
    payload_fwd.push_back(static_cast<std::uint8_t>(payload[i]));
  }
  std::string topic_fwd{topic};
  send(topic_fwd, payload_fwd);
}
void Client::send(Topic const &topic, std::string const &payload) {
  send(topic, payload.c_str());
}

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
