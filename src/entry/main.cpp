#include <entry.hpp>
#include <runtime_factory.hpp>
#include <signal.hpp>

#include <mqtt/async_client.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <chrono>

int main(int argc, char *argv[]) {
  using namespace std::chrono_literals;

  if (argc != 2) {
    spdlog::error("Usage: {} <path-to-config-file>", argv[0]);
    return 1;
  }

  try {
    HomeAutomation::System::initQuitCondition();

    auto runtime = HomeAutomation::Runtime::RuntimeFactory::fromFile(argv[1]);

    spdlog::info("Starting runtime");
    runtime->start(HomeAutomation::System::quitCondition);
    return runtime->wait();
  } catch (YAML::Exception const &exc) {
    spdlog::error("Could not parse configuration file: {}", exc.what());
    return 1;
  } catch (mqtt::exception &exc) {
    spdlog::error(
        "Some error occurred when interacting with the MQTT broker: {}",
        exc.what());
    return 1;
  }
}
