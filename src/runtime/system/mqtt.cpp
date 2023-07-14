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
  send(Message{topic, Payload{payload, payload + std::strlen(payload)}});
}
void Client::send(Topic const &topic, std::string const &payload) {
  send(topic, payload.c_str());
}

} // namespace MQTT
} // namespace IO
} // namespace HomeAutomation
