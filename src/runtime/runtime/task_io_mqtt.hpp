#pragma once

#include <gv.hpp>
#include <mqtt.hpp>
#include <task_io.hpp>

namespace HomeAutomation {

namespace Runtime {

using MQTTTopic = std::string;
using InputMapping = std::map<MQTTTopic, VarName>;
using OutputMapping = std::map<VarName, MQTTTopic>;

class MQTTIOLogic final : public HomeAutomation::Runtime::TaskIOLogic {
public:
  MQTTIOLogic(InputMapping &&inputs, OutputMapping &&outputs,
              HomeAutomation::GV *gv,
              std::shared_ptr<HomeAutomation::IO::MQTT::Client> mqttClient)
      : inputs{std::move(inputs)}, outputs{std::move(outputs)}, gv{gv},
        mqttClient{mqttClient}, outputValues{} {}

  void init() override;
  void shutdown() override;
  void before() override;
  void after() override;

private:
  InputMapping inputs;
  OutputMapping outputs;
  HomeAutomation::GV *gv;
  std::shared_ptr<HomeAutomation::IO::MQTT::Client> mqttClient;
  // as only changes in output values will be published,
  // previous values need to be remembered
  HomeAutomation::GvSegment outputValues;
};

} // namespace Runtime
} // namespace HomeAutomation
