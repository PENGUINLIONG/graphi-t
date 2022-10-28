#include "gft/assert.hpp"
#include "gft/log.hpp"
#include "gft/test.hpp"
#include "gft/json-serde.hpp"

enum class TestEnum {
  _123 = 123,
};

struct TestStructure {
  uint32_t a;
  bool b;
  std::string c;
  std::pair<std::string, int32_t> d;
  std::unique_ptr<uint8_t> e;
  std::map<uint64_t, std::string> f;
  std::unordered_map<uint64_t, std::string> g;
  std::vector<int32_t> h;
  std::array<int16_t, 3> i;
  std::uint16_t j[3];
  TestEnum k;
  std::optional<int64_t> l;
  uint64_t m;

  L_JSON_SERDE_FIELDS(a, b, c, d, e, f, g, h, i, j, k, l, m);
};

L_TEST(TestJsonSerde) {
  using namespace liong;
  using namespace liong::json;
  TestStructure ts1 {};
  ts1.a = 123;
  ts1.b = true;
  ts1.c = "123";
  ts1.d = std::make_pair<std::string, uint32_t>("12", 3);
  ts1.e = std::make_unique<uint8_t>(123);
  ts1.f[12] = "3";
  ts1.g[1] = "23";
  ts1.h.emplace_back(123);
  ts1.i = { 1, 2, 3 };
  ts1.j[0] = 1;
  ts1.j[1] = 2;
  ts1.j[2] = 3;
  ts1.k = TestEnum::_123;
  ts1.l = 123;
  ts1.m = 123123123123123123;

  JsonValue j1 = json::serialize(ts1);
  std::string json_lit = json::print(j1);
  L_INFO(json_lit);
  JsonValue j2 = json::parse(json_lit);
  TestStructure ts2 {};
  json::deserialize(j2, ts2);
  L_ASSERT(json_lit == json::print(json::serialize(ts2)));
  L_ASSERT(ts1.m == ts2.m); // Large integers should not be cast to double.
}
