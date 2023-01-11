#include <program_factory.hpp>
#include <runtime_factory.hpp>
#include <signal.hpp>

#include <mqtt/async_client.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <chrono>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    spdlog::error("Usage: {} <path-to-config-file>", argv[0]);
    return 1;
  }

  try {
    auto gv = HomeAutomation::GV{};
    auto scheduler = HomeAutomation::Runtime::Scheduler{};

    HomeAutomation::System::initQuitCondition([&scheduler](int sig) {
      (void)sig;
      scheduler.stop();
    });

    HomeAutomation::Runtime::RuntimeFactory::fromFile(argv[1], &gv, &scheduler);

    spdlog::info("Starting runtime");
    scheduler.start(&gv);
    return scheduler.wait();
  } catch (YAML::Exception const &exc) {
    spdlog::error("Could not parse configuration file: {}", exc.what());
    return 1;
  } catch (mqtt::exception const &exc) {
    spdlog::error(
        "Some error occurred when interacting with the MQTT broker: {}",
        exc.what());
    return 1;
  }
}
