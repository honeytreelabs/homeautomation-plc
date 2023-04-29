#pragma once

#include <common.hpp>

#include <fsm.hpp>
#include <trigger.hpp>

#include <spdlog/spdlog.h>

#include <cstdint>
#include <tuple>

namespace HomeAutomation {
namespace Library {

struct BlindIOs {
  bool up;
  bool down;
};
// The BlindOutputs tuple exists in order for easier mapping to Lua data types
using BlindOutputs = std::tuple<bool, bool>;

struct BlindConfig {
  std::chrono::milliseconds periodIdle;
  std::chrono::milliseconds periodUp;
  std::chrono::milliseconds periodDown;
};

// TODO: use std::chrono types and create wrapper in Lua bindings
BlindConfig BlindConfigFromMillis(std::uint32_t periodIdle,
                                  std::uint32_t periodUp,
                                  std::uint32_t periodDown);

struct BlindContext {
  BlindConfig const &cfg;
  TimeStamp now;
  BlindIOs inputs;
  BlindIOs outputs;
};

struct BlindState {
  BlindState(TimeStamp now, bool up, bool down)
      : start{now}, up_trigger{up}, down_trigger{down} {}

  virtual ~BlindState() = default;

  TimeStamp start;
  R_TRIG up_trigger;
  R_TRIG down_trigger;
};
struct BlindStateIdle;
struct BlindStateUp;
struct BlindStateDown;

using StateVariant = std::variant<BlindStateIdle, BlindStateUp, BlindStateDown>;
using OptionalStateVariant = std::optional<StateVariant>;

struct BlindStateIdle : public BlindState {
  BlindStateIdle(TimeStamp now) : BlindState(now, false, false) {
    spdlog::info("BlindStateIdle()");
  }
  void update(BlindContext &context) {
    context.outputs.up = false;
    context.outputs.down = false;
  }
};

struct BlindStateUp : public BlindState {
  BlindStateUp(TimeStamp now, bool up, bool down) : BlindState(now, up, down) {
    spdlog::info("BlindStateUp()");
  }
  void update(BlindContext &context) {
    context.outputs.up = true;
    context.outputs.down = false;
  }
};
struct BlindStateDown : public BlindState {
  BlindStateDown(TimeStamp now, bool up, bool down)
      : BlindState(now, up, down) {
    spdlog::info("BlindStateDown()");
  }
  void update(BlindContext &context) {
    context.outputs.up = false;
    context.outputs.down = true;
  }
};

OptionalStateVariant transition(BlindStateIdle &idle, BlindContext &context);
OptionalStateVariant transition(BlindStateUp &up, BlindContext &context);
OptionalStateVariant transition(BlindStateDown &down, BlindContext &context);

} // namespace Library
} // namespace HomeAutomation
