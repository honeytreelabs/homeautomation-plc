#pragma once

#include <i2c_bus.hpp>

#include <bithelpers.hpp>

#include <array>
#include <cstdint>

namespace HomeAutomation {
namespace IO {
namespace I2C {

// https://datasheets.maximintegrated.com/en/ds/MAX7311.pdf
class MAX7311Output : public HomeAutomation::IO::I2C::OutputModule {
public:
  MAX7311Output(std::uint8_t address)
      : address(address), outputs{}, last_outputs(outputs) {}
  void init(IReadWrite *io) override {
    // Port 1 configuration
    std::array<std::uint8_t, 2> configCmd{REGISTER_PORT_1_CONFIGURATION, 0x00};
    io->write(address, configCmd);

    std::array<std::uint8_t, 1> register1OutputCmd{REGISTER_OUTPUT_PORT_1};
    io->write(address, register1OutputCmd);

    std::array<std::uint8_t, 1> bytes{0x00};
    io->read(address, bytes);
    outputs = BitHelpers::bitflip(bytes[0]); // bits are inverted
  }

  std::uint8_t getOutputs() const { return outputs; }
  void setOutput(std::uint8_t pos, bool value) override {
    outputs = BitHelpers::bitset(outputs, pos, value);
  }
  void write(HomeAutomation::IO::I2C::IReadWrite *io) override {
    // writing is only necessary if there is change in outputs states
    if (outputs == last_outputs) {
      return;
    }

    // bits are inverted
    std::array<std::uint8_t, 2> bytes{REGISTER_OUTPUT_PORT_1,
                                      BitHelpers::bitflip(outputs)};
    io->write(address, bytes);

    last_outputs = outputs;
  }

private:
  static constexpr std::uint8_t const REGISTER_OUTPUT_PORT_1 = 0x02;
  static constexpr std::uint8_t const REGISTER_PORT_1_CONFIGURATION = 0x06;

  std::uint8_t const address;
  std::uint8_t outputs;
  std::uint8_t last_outputs;
};

// https://www.nxp.com/docs/en/data-sheet/PCF8574_PCF8574A.pdf
class PCF8574Input : public HomeAutomation::IO::I2C::InputModule {
public:
  PCF8574Input(std::uint8_t address) : address(address), inputs{0x00} {}
  void init(IReadWrite *io) override { read(io); }
  std::uint8_t getInputs() const { return inputs; };
  bool getInput(std::uint8_t pos) const override {
    return BitHelpers::bitget(inputs, pos);
  }

  void read(HomeAutomation::IO::I2C::IReadWrite *io) override {
    std::array<std::uint8_t, 1> bytes{0x00};
    io->read(address, bytes);

    inputs = BitHelpers::bitflip(bytes[0]); // bits are inverted
  }

private:
  std::uint8_t const address;
  std::uint8_t inputs;
};

class PCF8574Output : public HomeAutomation::IO::I2C::OutputModule {
public:
  PCF8574Output(std::uint8_t address)
      : address{address}, outputs{}, last_outputs{outputs} {}
  void init(IReadWrite *io) override {
    std::array<std::uint8_t, 1> bytes{0x00};
    io->read(address, bytes);
    outputs = BitHelpers::bitflip(bytes[0]); // bits are inverted
  }
  void write(IReadWrite *io) override {
    if (outputs == last_outputs) {
      return;
    }
    std::array<std::uint8_t, 1> bytes{
        BitHelpers::bitflip(outputs)}; // bits are inverted
    io->write(address, bytes);
    last_outputs = outputs;
  }
  void setOutput(std::uint8_t pos, bool value) override {
    outputs = BitHelpers::bitset(outputs, pos, value);
  }

private:
  std::uint8_t address;
  std::uint8_t outputs;
  std::uint8_t last_outputs;
};

} // namespace I2C
} // namespace IO
} // namespace HomeAutomation
