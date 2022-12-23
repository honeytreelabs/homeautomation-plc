#pragma once

#include <io_if.hpp>

#include <modbus.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace HomeAutomation {
namespace IO {
namespace Modbus {

class DigitalInputModule : public HomeAutomation::IO::DigitalInputModule {
public:
  virtual ~DigitalInputModule() = default;
  virtual void init(modbus_t *ctx) = 0;
  virtual void read(modbus_t *ctx) = 0;
};

class DigitalOutputModule : public HomeAutomation::IO::DigitalOutputModule {
public:
  virtual ~DigitalOutputModule() = default;
  virtual void init(modbus_t *ctx) = 0;
  virtual void write(modbus_t *ctx) = 0;
};

class BusRTU : public HomeAutomation::IO::Bus {
public:
  BusRTU(std::string const &path, int baud, char parity, int data_bit,
         int stop_bit)
      : path{path}, baud{baud}, parity{parity}, data_bit{data_bit},
        stop_bit{stop_bit}, modbus_ctx{nullptr}, inputs{}, outputs{} {}
  virtual ~BusRTU() = default;

  void RegisterInput(std::shared_ptr<DigitalInputModule> modbusModule) {
    inputs.push_back(modbusModule);
  }

  void RegisterOutput(std::shared_ptr<DigitalOutputModule> modbusModule) {
    outputs.push_back(modbusModule);
  }

  void init() override {
    spdlog::info(
        "Initializing Modbus RTU on {} with {} baud and {}{}{} setting.", path,
        baud, data_bit, parity, stop_bit);
    modbus_ctx = modbus_new_rtu(path.c_str(), baud, parity, data_bit, stop_bit);
    if (!modbus_ctx) {
      throw std::runtime_error("cannot open modbus bus");
    }
    int rc = modbus_connect(modbus_ctx);
    if (rc == -1) {
      std::stringstream s;
      s << "could not connect modbus device: " << std::strerror(errno);
      throw std::runtime_error(s.str());
    }
  }
  void close() override {
    modbus_close(modbus_ctx);
    modbus_free(modbus_ctx);
  }
  void readInputs() override {
    for (auto input : inputs) {
      input->read(modbus_ctx);
    }
  }

  void writeOutputs() override {
    for (auto output : outputs) {
      output->write(modbus_ctx);
    }
  }

protected:
  std::string const path;
  int baud;
  char parity;
  int data_bit;
  int stop_bit;
  modbus_t *modbus_ctx;

private:
  std::vector<std::shared_ptr<DigitalInputModule>> inputs;
  std::vector<std::shared_ptr<DigitalOutputModule>> outputs;
};

} // namespace Modbus
} // namespace IO
} // namespace HomeAutomation
