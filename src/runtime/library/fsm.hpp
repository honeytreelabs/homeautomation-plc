#pragma once

#include <common.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace HomeAutomation {

template <class Context, class... States> class FSM {
public:
  virtual ~FSM() = default;

  using StateVariant = std::variant<States...>;
  using OptionalStateVariant = std::optional<StateVariant>;

  FSM(StateVariant &&initialState, Context &&context)
      : curState{std::move(initialState)}, context_{std::move(context)} {}
  void update(Context &context) {
    auto newState = std::visit(
        [&context](auto &state) -> OptionalStateVariant {
          return transition(state, context);
        },
        curState);
    if (newState) {
      std::visit([&context](auto &state) { state.update(context); },
                 newState.value());
      curState = std::move(newState.value());
    }
  }

  Context &context() { return context_; }

private:
  StateVariant curState;
  Context context_;
};

} // namespace HomeAutomation
