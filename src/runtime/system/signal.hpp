#pragma once

#include <functional>

namespace HomeAutomation {
namespace System {

using SigHandlerCb = std::function<void(int sig)>;

void initQuitCondition(SigHandlerCb handler);

} // namespace System
} // namespace HomeAutomation
