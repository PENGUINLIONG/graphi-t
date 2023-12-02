#include "gft/stream.hpp"

#include "gft/assert.hpp"
#include "gft/log.hpp"
#include "gft/test.hpp"

using namespace liong;

L_TEST(StreamReadWriteRoundTrip) {
  stream::WriteStream ws;
  ws.append<uint8_t>(123);
  ws.append<uint32_t>(123);
  ws.append<double>(123);

  std::vector<uint8_t> data = ws.take();
  stream::ReadStream rs(data.data(), data.size());
  L_ASSERT(rs.extract<uint8_t>(), 123);
  L_ASSERT(rs.extract<uint32_t>(), 123);
  L_ASSERT(rs.extract<double>(), 123);
}

L_TEST(StreamReadWriteStructRoundTrip) {
  struct X {
    uint8_t a = 123;
    // Padding is potentially added here.
    uint32_t b = 123;
    // Padding is potentially added here.
    double c = 123;
  } xw, xr;

  stream::WriteStream ws;
  ws.append(xw);

  std::vector<uint8_t> data = ws.take();
  stream::ReadStream rs(data.data(), data.size());
  xr = rs.extract<X>();
  L_ASSERT(xw.a == xr.a);
  L_ASSERT(xw.b == xr.b);
  L_ASSERT(xw.c == xr.c);
}
