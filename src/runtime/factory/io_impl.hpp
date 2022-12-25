#pragma once

#include <io_if.hpp>
#include <scheduler_impl.hpp>

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

class BusIOLogic : public HomeAutomation::Scheduler::TaskIOLogic {
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

static VarValue &createMissingGVBool(HomeAutomation::GvSegment &gvSegment,
                                     std::string const &name) {
  gvSegment[name] = false;
  return gvSegment[name];
}

template <typename SequenceType, typename IOType>
static void insertCopySequenceBool(SequenceType &sequence,
                                   HomeAutomation::GvSegment &gvSegment,
                                   std::shared_ptr<IOType> io,
                                   YAML::Node const &node) {
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    auto const gvName = it->second.as<std::string>();
    uint8_t pin = it->first.as<uint8_t>();

    auto &gvVal = createMissingGVBool(gvSegment, gvName);
    sequence.emplace_back(io, &gvVal, pin);
  }
}

} // namespace Runtime
} // namespace HomeAutomation
