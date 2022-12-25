#pragma once

#include <gv.hpp>
#include <io_if.hpp>
#include <task_io.hpp>

namespace HomeAutomation {

namespace Runtime {

struct CopyInstructionInput {
  std::shared_ptr<HomeAutomation::IO::DigitalInputModule> input;
  HomeAutomation::VarValue *value;
  std::uint8_t pin;
};
using CopySequenceInput = std::list<CopyInstructionInput>;
struct CopyInstructionOutput {
  std::shared_ptr<HomeAutomation::IO::DigitalOutputModule> output;
  HomeAutomation::VarValue *value;
  std::uint8_t pin;
};
using CopySequenceOutput = std::list<CopyInstructionOutput>;

class BusIOLogic : public HomeAutomation::Runtime::TaskIOLogic {
public:
  BusIOLogic(std::shared_ptr<HomeAutomation::IO::Bus> bus,
             CopySequenceInput &&inputSequence,
             CopySequenceOutput &&outputSequence)
      : bus{bus}, inputSequence{std::move(inputSequence)},
        outputSequence{std::move(outputSequence)} {}
  virtual ~BusIOLogic() = default;

  void init() override { bus->init(); }

  void shutdown() override { bus->close(); }

  void before() override {
    bus->readInputs();
    for (auto &instr : inputSequence) {
      *instr.value = instr.input->getInput(instr.pin);
    }
  }

  void after() override {
    for (auto &instr : outputSequence) {
      instr.output->setOutput(instr.pin, std::get<bool>(*instr.value));
    }
    bus->writeOutputs();
  }

private:
  std::shared_ptr<HomeAutomation::IO::Bus> bus;
  CopySequenceInput inputSequence;
  CopySequenceOutput outputSequence;
};

} // namespace Runtime
} // namespace HomeAutomation
