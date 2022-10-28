#include "gft/log.hpp"
#include "gft/test.hpp"

using namespace liong;

int main(int argc, char** argv) {
  try {
    test::TestRegistry::run_all();
  } catch (const std::exception& e) {
    L_ERROR("application threw an exception");
    L_ERROR(e.what());
    L_ERROR("application cannot continue");
    return -1;
  } catch (...) {
    L_ERROR("application threw an illiterate exception");
    return -1;
  }

  return 0;
}
