#pragma once

#include <modbus.hpp>

#include <spdlog/spdlog.h>

#include <array>
#include <cstring>

namespace HomeAutomation {
namespace IO {
namespace Modbus {

// this class only uses the upper 8 outputs of the available 16
class WP8026ADAM final : public DigitalInputModule {
public:
  WP8026ADAM(int slave) : slave{slave}, inputs{} {}
  virtual ~WP8026ADAM() = default;
  bool getInput(std::uint8_t pos) const override {
    if (pos >= inputs.size()) {
      throw std::runtime_error("invalid position given for WP8026ADAM");
    }
    return inputs[pos];
  }
  void init(modbus_t *ctx) override { read(ctx); };
  void read(modbus_t *ctx) override {
    int rc = modbus_set_slave(ctx, slave);
    if (rc == -1) {
      spdlog::error("could not set modbus slave address: {}",
                    std::strerror(errno));
    }
    // skip the first 8 digital inputs for now
    rc = modbus_read_input_bits(ctx, ADDR, inputs.size(), inputs.data());
    if (rc == -1) {
      spdlog::error("could not read from modbus: {}", std::strerror(errno));
      return;
    }
  };

private:
  static constexpr int const ADDR = 0x0008;
  int const slave;
  std::array<std::uint8_t, 8> inputs;
};

class R4S8CRMB final : public DigitalOutputModule {
public:
  R4S8CRMB(int slave) : slave{slave}, outputs{}, last_outputs{} {}
  virtual ~R4S8CRMB() = default;
  void setOutput(std::uint8_t pos, bool value) override {
    if (pos >= outputs.size()) {
      throw std::runtime_error("invalid position given for R4S8CRMB");
    }
    outputs[pos] = value ? TRUE : FALSE;
  }
  void init(modbus_t *ctx) override { write(ctx); };
  void write(modbus_t *ctx) override {
    if (outputs == last_outputs) {
      return;
    }
    int rc = modbus_set_slave(ctx, slave);
    if (rc == -1) {
      spdlog::error("could not set modbus slave address: {}",
                    std::strerror(errno));
    }
    rc = modbus_write_bits(ctx, ADDR, outputs.size(), outputs.data());
    if (rc == -1) {
      spdlog::error("could not write to modbus: {}", std::strerror(errno));
      return;
    }
    last_outputs = outputs;
  }

private:
  static constexpr int const ADDR = 0x0000;
  int const slave;
  std::array<std::uint8_t, 8> outputs;
  std::array<std::uint8_t, 8> last_outputs;
};

} // namespace Modbus
} // namespace IO
} // namespace HomeAutomation
