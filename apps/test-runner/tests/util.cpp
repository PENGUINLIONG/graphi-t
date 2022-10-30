#include "gft/assert.hpp"
#include "gft/log.hpp"
#include "gft/test.hpp"
#include "gft/util.hpp"

L_TEST(Crc32Correctness) {
  std::string data = "penguinliongpenguinliongpenguinliongpenguinliong";
  uint32_t x = liong::util::crc32(data.data(), data.size());
  L_ASSERT(x == 0xc4c82680);
}
