#pragma once

#include <fsm.hpp>
#include <trigger.hpp>

#include <spdlog/spdlog.h>

#include <chrono>

using namespace std::literals;

namespace HomeAutomation {
namespace Components {

struct BlindOutputs {
  bool up;
  bool down;
};

inline bool operator==(BlindOutputs const &lhs, BlindOutputs const &rhs) {

  return lhs.up == rhs.up && lhs.down == rhs.down;
}

struct BlindConfig {
  std::chrono::milliseconds periodIdle;
  std::chrono::milliseconds periodUp;
  std::chrono::milliseconds periodDown;
};

using BlindState = State<BlindOutputs, bool, bool>;
using UniqueBlindState = std::unique_ptr<BlindState>;
class BlindFSM : public FSM<BlindOutputs, bool, bool> {
public:
  BlindFSM(UniqueBlindState state) : FSM(std::move(state)) {}
};
using OptionalBlindState = std::optional<UniqueBlindState>;
constexpr const auto NoTransition = std::nullopt;

class BlindStateIdle : public BlindState {
public:
  BlindStateIdle(BlindConfig const &cfg, TimeStamp const &now)
      : BlindStateIdle(cfg, now, false, false) {}
  BlindStateIdle(BlindConfig const &cfg, TimeStamp const &now, bool up,
                 bool down)
      : cfg{cfg}, start{now}, up_trigger(up), down_trigger(down) {
    spdlog::info("BlindStateIdle()");
  }
  OptionalBlindState execute(TimeStamp now, bool up, bool down) override;
  BlindOutputs const getStateData() const override {
    return BlindOutputs{false, false};
  }
  std::string const id() const override { return std::string("idle"); }

private:
  BlindConfig const &cfg;
  TimeStamp start;

  R_TRIG up_trigger;
  R_TRIG down_trigger;
};

template <BlindOutputs const &StateData, StringLiteral ID>
class BlindStateMove : public BlindState {
public:
  BlindStateMove(BlindConfig const &cfg, TimeStamp const &now, bool up,
                 bool down)
      : cfg{cfg}, start{now}, up_trigger(up), down_trigger(down) {
    spdlog::info("BlindStateMove({})", id());
  }
  OptionalBlindState execute(TimeStamp now, bool up, bool down) override {
    spdlog::debug("BlindStateUp::execute");

    if (now - start > cfg.periodUp || up_trigger.execute(up) ||
        down_trigger.execute(down)) {
      return std::make_unique<BlindStateIdle>(cfg, now, up, down);
    }

    return NoTransition;
  }
  BlindOutputs const getStateData() const override { return StateData; }
  std::string const id() const override { return ID.value; }

private:
  BlindConfig const &cfg;
  TimeStamp start;

  R_TRIG up_trigger;
  R_TRIG down_trigger;
};

constexpr BlindOutputs const UpOutputs{true, false};
using BlindStateUp = BlindStateMove<UpOutputs, "up">;

constexpr BlindOutputs const DownOutputs{false, true};
using BlindStateDown = BlindStateMove<DownOutputs, "down">;

class Blind {
public:
  Blind(BlindConfig const &cfg, TimeStamp const &now);
  BlindOutputs execute(TimeStamp now, bool button_up, bool button_down);

private:
  BlindFSM fsm;
};

} // namespace Components
} // namespace HomeAutomation
