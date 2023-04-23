#pragma once

#include <common.hpp>

#include <fsm.hpp>
#include <trigger.hpp>

#include <hfsm2/machine.hpp>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <tuple>

namespace HomeAutomation {
namespace Library {

struct BlindIOs {
  bool up;
  bool down;
};
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
  BlindConfig const cfg;
  TimeStamp now;
  BlindIOs inputs;
  BlindIOs outputs;
};

struct BlindStateIdle;
struct BlindStateUp;
struct BlindStateDown;

// convenience typedef
using M = hfsm2::MachineT<hfsm2::Config::ContextT<BlindContext>>;

using BlindFSM = M::PeerRoot<BlindStateIdle, BlindStateUp, BlindStateDown>;

struct BlindState : BlindFSM::State {
  BlindState() = default;
  virtual ~BlindState() = default;
  void enter(Control &control) noexcept {
    start = control.context().now;
    up_trigger = R_TRIG{control.context().inputs.up};
    down_trigger = R_TRIG{control.context().inputs.down};
  }
  TimeStamp start;
  R_TRIG up_trigger;
  R_TRIG down_trigger;
};

struct BlindStateIdle : BlindState {
  BlindStateIdle() = default;

  void enter(Control &control) noexcept {
    spdlog::info("BlindStateIdle()");
    control.context().outputs = {false, false};
    BlindState::enter(control);
  }

  void update(FullControl &control) noexcept {
    auto up_triggered = up_trigger.execute(control.context().inputs.up);
    auto down_triggered = up_trigger.execute(control.context().inputs.down);

    auto diff = control.context().now - start;
    if (diff < control.context().cfg.periodIdle) {
      return;
    }

    if (up_triggered) {
      control.changeTo<BlindStateUp>();
    } else if (down_triggered) {
      control.changeTo<BlindStateDown>();
    }
  }
};

struct BlindStateUp : BlindState {
  BlindStateUp() = default;

  void enter(Control &control) noexcept {
    spdlog::info("BlindStateUp()");
    control.context().outputs = {true, false};
    BlindState::enter(control);
  }

  void update(FullControl &control) noexcept {
    if (control.context().now - start > control.context().cfg.periodUp ||
        up_trigger.execute(control.context().inputs.up) ||
        down_trigger.execute(control.context().inputs.down)) {
      control.changeTo<BlindStateIdle>();
    }
  }
};

struct BlindStateDown : BlindState {
  BlindStateDown() = default;

  void enter(Control &control) noexcept {
    spdlog::info("BlindStateDown()");
    control.context().outputs = {false, true};
    BlindState::enter(control);
  }

  void update(FullControl &control) noexcept {
    if (control.context().now - start > control.context().cfg.periodDown ||
        up_trigger.execute(control.context().inputs.up) ||
        down_trigger.execute(control.context().inputs.down)) {
      control.changeTo<BlindStateIdle>();
    }
  }
};

constexpr hfsm2::StateID const BlindStateIdleID = 1;
static_assert(BlindFSM::stateId<BlindStateIdle>() == BlindStateIdleID, "");

} // namespace Library
} // namespace HomeAutomation
