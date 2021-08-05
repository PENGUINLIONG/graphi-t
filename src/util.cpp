#include "util.hpp"
#include "assert.hpp"

namespace liong {

namespace util {

std::vector<uint8_t> load_file(const char* path) {
  std::ifstream f(path, std::ios::ate | std::ios::binary | std::ios::in);
  assert(f.is_open(), "unable to open file: ", path);
  size_t size = f.tellg();
  f.seekg(std::ios::beg);
  std::vector<uint8_t> buf;
  buf.resize(size);
  f.read((char*)buf.data(), size);
  f.close();
  return buf;
}
std::string load_text(const char* path) {
  std::ifstream f(path, std::ios::ate | std::ios::binary | std::ios::in);
  assert(f.is_open(), "unable to open file: ", path);
  size_t size = f.tellg();
  f.seekg(std::ios::beg);
  std::string buf;
  buf.reserve(size + 1);
  buf.resize(size);
  f.read((char*)buf.data(), size);
  f.close();
  return buf;
}
void save_file(const char* path, const void* data, size_t size) {
  std::ofstream f(path, std::ios::trunc | std::ios::out | std::ios::binary);
  assert(f.is_open(), "unable to open file: ", path);
  f.write((const char*)data, size);
  f.close();
}

std::map<std::string, std::string> load_ini(const char* path) {
  std::map<std::string, std::string> rv;

  auto txt = liong::util::load_text(path);
  auto beg = txt.begin();
  auto pos = beg;
  std::string key;
  while (pos != txt.end()) {
    char c = *pos;

    if (c == '\r' || c == '\n') {
      if (pos != beg) {
        liong::assert(!key.empty(), "unexpected end of line");
      } else {
        rv[std::move(key)] = std::string(beg, pos);
        beg = pos;
      }
    }

    if (key.empty() && c == '=') {
      key = std::string(beg, pos);
      beg = pos;
    }

    ++pos;
  }

  return rv;
}


} // namespace util

} // namespace liong
