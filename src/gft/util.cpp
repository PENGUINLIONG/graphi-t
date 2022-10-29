#include "gft/util.hpp"
#include "gft/assert.hpp"
#include <chrono>
#include <thread>

namespace liong {

namespace util {

std::vector<uint8_t> load_file(const char* path) {
  std::ifstream f(path, std::ios::ate | std::ios::binary | std::ios::in);
  L_ASSERT(f.is_open(), "unable to open file: ", path);
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
  L_ASSERT(f.is_open(), "unable to open file: ", path);
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
  L_ASSERT(f.is_open(), "unable to open file: ", path);
  f.write((const char*)data, size);
  f.close();
}
void save_text(const char* path, const std::string& txt) {
  std::ofstream f(path, std::ios::trunc | std::ios::out);
  L_ASSERT(f.is_open(), "unable to open file: ", path);
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
  L_ASSERT(size >= offset + n);
  offset += n;
  return *this;
}
void DataStream::extract_data(void* out, size_t size) {
  L_ASSERT(size_remain() >= size);
  const void* buf = (const uint8_t*)data() + offset;
  offset += size;
  std::memcpy(out, buf, size);
}

/*
** The crc32 is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
**
** Author:Wang Yaofu voipman@qq.com
** Description: The source file of class crc32.
**  CRC32 implementation according to IEEE standards.
**  Polynomials are represented in LSB-first form
**  following parameters:
**    Width                      : 32 bit
**    Poly                       : 0xedb88320
**    Output for "123456789"     : 0xCBF43926
*/
uint32_t crc32(const void* data, int size) {
  static uint32_t LUT[] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
  };
  int i;
  uint32_t crc32val = 0;
  crc32val ^= 0xFFFFFFFF;

  for (i = 0;  i < size;  i++) {
    uint32_t x = ((const uint8_t*)data)[i];
    crc32val = LUT[(crc32val ^ x) & 0xFF] ^ ((crc32val >> 8) & 0x00FFFFFF);
  }

  return crc32val ^ 0xFFFFFFFF;
}

} // namespace util

} // namespace liong
