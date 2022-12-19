#pragma once

#include <cstdint>

namespace HomeAutomation {
namespace IO {

class DigitalInputModule {
public:
  virtual ~DigitalInputModule() = default;
  virtual bool getInput(std::uint8_t pos) const = 0;
};

class DigitalOutputModule {
public:
  virtual ~DigitalOutputModule() = default;
  virtual void setOutput(std::uint8_t pos, bool value) = 0;
};

class Bus {
public:
  virtual ~Bus() = default;
  virtual void init() = 0;
  virtual void close() = 0;
  virtual void readInputs() = 0;
  virtual void writeOutputs() = 0;
};

} // namespace IO
} // namespace HomeAutomation
