#include <signal.hpp>

#include <csignal>

namespace HomeAutomation {
namespace System {

constexpr std::sig_atomic_t const NO_SIGNAL = 0;
static std::sig_atomic_t signalStatus = NO_SIGNAL;

static void sigHandler(int sigNo) { signalStatus = sigNo; }

void initQuitCondition() {
  std::signal(SIGINT, sigHandler);
  std::signal(SIGTERM, sigHandler);
}
bool quitCondition() { return signalStatus != NO_SIGNAL; }

} // namespace System
} // namespace HomeAutomation
