// Uncompressed Zip archive I/O.
// @PENGUINLIONG
#pragma once
#include <string>
#include <vector>
#include <map>

namespace liong {
namespace zip {

struct ZipFileRecord {
  std::string file_name;
  const void* data;
  size_t size;
  uint32_t crc32;
};

struct ZipArchive {
  std::vector<ZipFileRecord> records;
  std::map<std::string, size_t> file_name2irecord;

  static ZipArchive from_bytes(const uint8_t* data, size_t size);
  static ZipArchive from_bytes(const std::vector<uint8_t>& out);

  // `data` has to be kept alive through out the archive's lifetime.
  void add_file(const std::string& file_name, const void* data, size_t size);
  void to_bytes(std::vector<uint8_t>& out) const;
};

} // namespace zip
} // namespace liong
