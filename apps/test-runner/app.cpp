#include "gft/log.hpp"
#include "gft/test.hpp"

using namespace liong;

int main(int argc, char** argv) {
  try {
    test::TestRegistry::run_all();
  } catch (const std::exception& e) {
    log::error("application threw an exception");
    log::error(e.what());
    log::error("application cannot continue");
    return -1;
  } catch (...) {
    log::error("application threw an illiterate exception");
    return -1;
  }

  return 0;
}
