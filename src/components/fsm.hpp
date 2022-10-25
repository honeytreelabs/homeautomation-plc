#pragma once

#include <common.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace HomeAutomation {

template <typename RetType, typename... Types> class FSM;

template <typename RetType, typename... Types> class State {
public:
  using OptionalState = std::optional<std::unique_ptr<State>>;

  virtual ~State() = default;
  virtual OptionalState execute(TimeStamp now, Types... args) = 0;
  virtual RetType const getStateData() const = 0;
  virtual std::string const id() const = 0;
};

template <typename RetType, typename... Types> class FSM {
public:
  using UniqueState = std::unique_ptr<State<RetType, Types...>>;

  FSM(UniqueState initialState) : state(std::move(initialState)) {}
  virtual ~FSM() {}
  void execute(TimeStamp now, Types... args) {
    auto nextState = state->execute(now, args...);
    if (!nextState) {
      return;
    }
    state = std::move(nextState.value());
  }
  RetType getStateData() { return state->getStateData(); }

private:
  UniqueState state;
};

} // namespace HomeAutomation
