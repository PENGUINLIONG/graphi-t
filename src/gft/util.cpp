#include "gft/util.hpp"
#include "gft/assert.hpp"
#include <chrono>
#include <thread>

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
void save_text(const char* path, const std::string& txt) {
  std::ofstream f(path, std::ios::trunc | std::ios::out);
  assert(f.is_open(), "unable to open file: ", path);
  f.write(txt.c_str(), txt.size());
  f.close();
}

// Save an array of 8-bit unsigned int colors with RGBA channels packed from LSB
// to MSB in a 32-bit unsigned int into a bitmap file.
void save_bmp(
  const uint32_t* pxs,
  uint32_t w,
  uint32_t h,
  const char* path
) {
  std::fstream f(path, std::ios::out | std::ios::binary | std::ios::trunc);
  f.write("BM", 2);
  uint32_t img_size = w * h * sizeof(uint32_t);
  uint32_t bmfile_hdr[] = { 14 + 108 + img_size, 0, 14 + 108 };
  f.write((const char*)bmfile_hdr, sizeof(bmfile_hdr));
  uint32_t bmcore_hdr[] = {
    108, w, h, 1 | (32 << 16), 3, img_size, 2835, 2835, 0, 0,
    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, 0x57696E20,
    0,0,0,0,0,0,0,0,0,0,0,0,
  };
  f.write((const char*)bmcore_hdr, sizeof(bmcore_hdr));
  uint32_t buf;
  for (auto i = 0; i < h; ++i) {
    for (auto j = 0; j < w; ++j) {
      buf = pxs[(h - i - 1) * w + j];
      f.write((const char*)&buf, sizeof(uint32_t));
    }
  }
  f.flush();
  f.close();
}
// Save an array of 32-bit floating point colors with RGBA channels into a
// bitmap file.
void save_bmp(
  const float* pxs,
  uint32_t w,
  uint32_t h,
  const char* path
) {
  std::vector<uint32_t> packed_pxs;
  packed_pxs.resize(w * h * 4);
  size_t npx = (size_t)w * (size_t)h;
  for (size_t i = 0; i < npx; ++i) {
    uint32_t r = (uint32_t)((float)pxs[i * 4 + 0] * 255.0f);
    uint32_t g = (uint32_t)((float)pxs[i * 4 + 1] * 255.0f);
    uint32_t b = (uint32_t)((float)pxs[i * 4 + 2] * 255.0f);
    uint32_t a = (uint32_t)((float)pxs[i * 4 + 3] * 255.0f);
    packed_pxs[i] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
  }
  save_bmp(packed_pxs.data(), w, h, path);
}

void sleep_for_us(uint64_t t) {
  std::this_thread::sleep_for(std::chrono::microseconds(t));
}

bool starts_with(const std::string& start, const std::string& str) {
  if (str.size() < start.size()) { return false; }
  for (size_t i = 0; i < start.size(); ++i) {
    if (str[i] != start[i]) {
      return false;
    }
  }
  return true;
}
bool ends_with(const std::string& end, const std::string& str) {
  if (str.size() < end.size()) { return false; }
  size_t offset = str.size() - end.size();
  for (size_t i = 0; i < end.size(); ++i) {
    if (str[offset + i] != end[i]) {
      return false;
    }
  }
  return true;
}
std::vector<std::string> split(char sep, const std::string& str) {
  std::vector<std::string> out;

  auto beg = str.begin();
  auto pos = beg;
  auto end = str.end();

  while (pos != end) {
    if (*pos == sep) {
      if (beg != pos) {
        out.emplace_back(beg, pos);
      }
      ++pos;
      beg = pos;
    } else {
      ++pos;
    }
  }
  if (beg != end) {
    out.emplace_back(beg, end);
  }

  return out;
}
std::string trim(const std::string& str) {
  auto beg = str.begin();
  auto end = str.end();

  char c;
  while (beg != end) {
    c = *beg;
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      ++beg;
    } else {
      break;
    }
  }
  auto next_end = end;
  while (beg != end) {
    c = *(--next_end);
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      end = next_end;
    } else {
      break;
    }
  }
  return std::string(beg, end);
}


DataStream& DataStream::skip(size_t n) {
  assert(size >= offset + n);
  offset += n;
  return *this;
}
void DataStream::extract_data(void* out, size_t size) {
  assert(size_remain() >= size);
  const void* buf = (const uint8_t*)data() + offset;
  offset += size;
  std::memcpy(out, buf, size);
}

} // namespace util

} // namespace liong
