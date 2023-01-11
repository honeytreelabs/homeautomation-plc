#include <signal.hpp>

#include <csignal>

namespace HomeAutomation {
namespace System {

static SigHandlerCb sigHandlerCb;

static void sigHandler(int sigNo) { sigHandlerCb(sigNo); }

void initQuitCondition(SigHandlerCb handler) {
  sigHandlerCb = handler;
  std::signal(SIGINT, sigHandler);
  std::signal(SIGTERM, sigHandler);
}

} // namespace System
} // namespace HomeAutomation
