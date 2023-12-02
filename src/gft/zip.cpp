#include "gft/assert.hpp"
#include "gft/log.hpp"
#include "gft/zip.hpp"
#include "gft/util.hpp"
#include "gft/stream.hpp"

namespace liong {
namespace zip {

enum ZipSignature {
  L_ZIP_SIGNATURE_LOCAL_FILE_HEADER = 0x04034b50,
  L_ZIP_SIGNATURE_DATA_DESCRIPTOR = 0x08074b50,
  L_ZIP_SIGNATURE_CENTRAL_DIRECTORY_FILE_HEADER = 0x02014b50,
  L_ZIP_SIGNATURE_END_OF_CENTRAL_DIRECTORY_RECORD = 0x06054b50,
};

struct ZipParser {
  stream::ReadStream stream;
  std::vector<ZipFileRecord> records;
  std::map<uint32_t, size_t> rel_offset2irecord;
  std::map<std::string, size_t> file_name2irecord;
  uint32_t cdr_offset;

  ZipParser(const void* data, size_t size) : stream(data, size), cdr_offset() {}

  bool extract_local_file_header() {
    uint32_t rel_offset = stream.offset() - sizeof(uint32_t);

    if (stream.size_remain() < 30) {
      L_ERROR("corrupted local file header");
      return false;
    }
    uint16_t min_version = stream.extract<uint16_t>();
    uint16_t flags = stream.extract<uint16_t>();
    uint16_t compression_method = stream.extract<uint16_t>();
    uint16_t last_modify_time = stream.extract<uint16_t>();
    uint16_t last_modify_date = stream.extract<uint16_t>();
    uint32_t crc32 = stream.extract<uint32_t>();
    uint32_t compressed_size = stream.extract<uint32_t>();
    uint32_t uncompressed_size = stream.extract<uint32_t>();
    if (compression_method != 0 || compressed_size != uncompressed_size) {
      L_ERROR("compressed zip is not supported");
      return false;
    }
    if (compressed_size == 0xFFFFFFFF || uncompressed_size == 0xFFFFFFFF) {
      L_ERROR("zip64 is not supported");
    }
    std::string file_name(stream.extract<uint16_t>(), '\0');
    std::vector<uint8_t> extra_field(stream.extract<uint16_t>());
    if (stream.size_remain() < file_name.size() + extra_field.size()) {
      L_ERROR("corrupted local file header");
      return false;
    }
    stream.extract_data(file_name.data(), file_name.size());
    stream.extract_data(extra_field.data(), extra_field.size());

    size_t irecord = records.size();

    rel_offset2irecord[rel_offset] = irecord;

    ZipFileRecord& record = records.emplace_back();
    record.file_name = file_name;
    record.data = stream.pos();
    record.size = uncompressed_size;
    record.crc32 = crc32;

    file_name2irecord[file_name] = irecord;

    stream.skip(compressed_size);

    if ((flags & 0x8) != 0) {
      uint32_t sig_dd = 0;
      if (stream.try_peek<uint32_t>(sig_dd)) {
        if (sig_dd == L_ZIP_SIGNATURE_DATA_DESCRIPTOR) {
          stream.skip(sizeof(uint32_t));
        }
      }
      extract_data_descriptor();
    }

    return true;
  }

  bool extract_data_descriptor() {
    if (records.empty()) {
      L_ERROR("cannot apply data descriptor before file entries");
      return false;
    }

    uint32_t crc32 = stream.extract<uint32_t>();
    uint32_t compressed_size = stream.extract<uint32_t>();
    uint32_t uncompressed_size = stream.extract<uint32_t>();

    ZipFileRecord& record = records.back();
    record.crc32 = crc32;
    record.size = uncompressed_size;
    return true;
  }

  bool extract_central_directory_file_header() {
    uint16_t version = stream.extract<uint16_t>();
    uint16_t min_version = stream.extract<uint16_t>();
    uint16_t flags = stream.extract<uint16_t>();
    uint16_t compression_method = stream.extract<uint16_t>();
    if (compression_method != 0) {
      L_ERROR("compressed zip is not supported");
      return false;
    }
    uint16_t last_modify_time = stream.extract<uint16_t>();
    uint16_t last_modify_date = stream.extract<uint16_t>();
    uint32_t crc32 = stream.extract<uint32_t>();
    uint32_t compressed_size = stream.extract<uint32_t>();
    uint32_t uncompressed_size = stream.extract<uint32_t>();
    std::string file_name(stream.extract<uint16_t>(), '\0');
    std::vector<uint8_t> extra_field(stream.extract<uint16_t>());
    std::string comment(stream.extract<uint16_t>(), '\0');
    uint16_t disk_number = stream.extract<uint16_t>();
    if (disk_number != 0) {
      L_ERROR("multi-disk zip file is not supported");
      return false;
    }
    uint16_t internal_attrs = stream.extract<uint16_t>();
    uint32_t external_attrs = stream.extract<uint32_t>();
    uint32_t rel_offset = stream.extract<uint32_t>();
    stream.extract_data(file_name.data(), file_name.size());
    stream.extract_data(extra_field.data(), extra_field.size());
    stream.extract_data(comment.data(), comment.size());

    auto it = rel_offset2irecord.find(rel_offset);
    if (it == rel_offset2irecord.end()) {
      L_ERROR("cannot find an file entry as specified in the cdr");
      return false;
    }

    const ZipFileRecord& record = records.at(it->second);
    if (record.file_name != file_name) {
      L_ERROR("zip file name in file entry mismatched the cdr");
      return false;
    }
    if (record.size != uncompressed_size) {
      L_ERROR("zip file size in file entry mismatched the cdr");
      return false;
    }
    if (record.crc32 != crc32) {
      L_ERROR("zip file crc32 in file entry mismatched the cdr");
      return false;
    }

    return true;
  }

  bool extract_end_of_central_directory_record() {
    uint16_t cur_disc_number = stream.extract<uint16_t>();
    uint16_t cdr_disk_number = stream.extract<uint16_t>();
    uint16_t ncdr_on_this_disk = stream.extract<uint16_t>();
    uint16_t ncdr_total = stream.extract<uint16_t>();
    if (cur_disc_number != 0 || cdr_disk_number != 0) {
      L_ERROR("multi-disk zip file is not supported");
      return false;
    }
    if (ncdr_on_this_disk != ncdr_total) {
      L_ERROR("multi-disk zip file is not supported");
      return false;
    }
    uint32_t cdr_size_total = stream.extract<uint32_t>();
    uint32_t cdr_offset = stream.extract<uint32_t>();
    std::string comment(stream.extract<uint16_t>(), '\0');
    stream.extract_data(comment.data(), comment.size());
    return true;
  }

  bool parse_file_records() {
    uint32_t sig = 0;
    while (stream.try_peek(sig)) {

      if (sig == L_ZIP_SIGNATURE_LOCAL_FILE_HEADER) {
        stream.skip(sizeof(sig));

        if (!extract_local_file_header()) {
          L_ERROR("failed to extract local file header");
          return false;
        }

      } else if (sig == L_ZIP_SIGNATURE_DATA_DESCRIPTOR) {
        stream.skip(sizeof(sig));

        if (!extract_data_descriptor()) {
          L_ERROR("cannot extract data descriptor");
          return false;
        }

      } else {
        break;
      }
    }
    return true;
  }

  bool parse_central_directory_records() {
    uint32_t sig = 0;
    while (stream.try_peek(sig)) {
      if (sig == L_ZIP_SIGNATURE_CENTRAL_DIRECTORY_FILE_HEADER) {
        stream.skip(sizeof(sig));

        if (!extract_central_directory_file_header()) {
          L_ERROR("cannot extract central directory file header");
          return false;
        }

      } else if (sig == L_ZIP_SIGNATURE_END_OF_CENTRAL_DIRECTORY_RECORD) {
        stream.skip(sizeof(sig));

        if (!extract_end_of_central_directory_record()) {
          L_ERROR("cannot extract end of central directory record");
          return false;
        }
        break;

      } else {
        L_ERROR("unexpected zip section signature");
        return false;
      }
    }
    return true;
  }

  bool parse() {
    bool out = true;
    out &= parse_file_records();
    cdr_offset = stream.offset();
    out &= parse_central_directory_records();
    return out;
  }
};

struct ZipArchiver {
  stream::WriteStream stream;
  const std::vector<ZipFileRecord>& records;
  std::vector<uint32_t> rel_offsets;
  uint32_t cdr_offset;
  uint32_t ecdr_offset;

  ZipArchiver(const std::vector<ZipFileRecord>& records) : records(records) {}

  void append_file_records() {
    for (size_t i = 0; i < records.size(); ++i) {
      rel_offsets.emplace_back((uint32_t)stream.size());

      const ZipFileRecord& record = records.at(i);
      stream.append<uint32_t>(L_ZIP_SIGNATURE_LOCAL_FILE_HEADER);
      stream.append<uint16_t>(0); // min version
      stream.append<uint16_t>(0); // flags
      stream.append<uint16_t>(0); // compression
      stream.append<uint16_t>(0); // last modify time
      stream.append<uint16_t>(0); // last modify date
      stream.append<uint32_t>(record.crc32);
      stream.append<uint32_t>(record.size); // compressed size
      stream.append<uint32_t>(record.size); // uncompressed size
      stream.append<uint16_t>((uint16_t)record.file_name.size());
      stream.append<uint16_t>(0); // extra field size
      stream.append_data(record.file_name.data(), record.file_name.size());

      stream.append_data(record.data, record.size);
    }
  }
  void append_central_directory_records() {
    uint32_t cdr_offset = (uint32_t)stream.size();

    for (size_t i = 0; i < records.size(); ++i) {
      const ZipFileRecord& record = records.at(i);
      stream.append<uint32_t>(L_ZIP_SIGNATURE_CENTRAL_DIRECTORY_FILE_HEADER);
      stream.append<uint16_t>(0); // version
      stream.append<uint16_t>(0); // min version
      stream.append<uint16_t>(0); // flags
      stream.append<uint16_t>(0); // compression
      stream.append<uint16_t>(0); // last modify time
      stream.append<uint16_t>(0); // last modify date
      stream.append<uint32_t>(record.crc32);
      stream.append<uint32_t>(record.size); // compressed size
      stream.append<uint32_t>(record.size); // uncompressed size
      stream.append<uint16_t>((uint16_t)record.file_name.size());
      stream.append<uint16_t>(0);                 // extra field size
      stream.append<uint16_t>(0);                 // comment size
      stream.append<uint16_t>(0);                 // disk number
      stream.append<uint16_t>(0);                 // internal attrs
      stream.append<uint32_t>(0);                 // external attrs
      stream.append<uint32_t>(rel_offsets.at(i)); // offset
      stream.append_data(record.file_name.data(), record.file_name.size());
    }
  }
  void append_end_of_central_directory_record() {
    uint32_t ecdr_offset = (uint32_t)stream.size();

    stream.append<uint32_t>(L_ZIP_SIGNATURE_END_OF_CENTRAL_DIRECTORY_RECORD);
    stream.append<uint16_t>(0);                        // current disk number
    stream.append<uint16_t>(0);                        // cdr disk number
    stream.append<uint16_t>(records.size());           // cdr count on this disk
    stream.append<uint16_t>(records.size());           // total cdr count
    stream.append<uint32_t>(ecdr_offset - cdr_offset); // cdr size
    stream.append<uint32_t>(cdr_offset);               // cdr offset
    stream.append<uint16_t>(0);                        // comment size
  }

  void archive() {
    append_file_records();
    append_central_directory_records();
    append_end_of_central_directory_record();
  }
};

ZipArchive ZipArchive::from_bytes(const uint8_t* data, size_t size) {
  ZipParser parser(data, size);
  if (!parser.parse()) {
    L_ERROR("failed to parse zip archive");
    return {};
  }

  ZipArchive ar {};
  ar.records = std::move(parser.records);
  ar.file_name2irecord = std::move(parser.file_name2irecord);
  return ar;
}
ZipArchive ZipArchive::from_bytes(const std::vector<uint8_t>& data) {
  return from_bytes(data.data(), data.size());
}


void ZipArchive::add_file(
  const std::string& file_name,
  const void* data,
  size_t size
) {
  ZipFileRecord record {};
  record.file_name = file_name;
  record.data = data;
  record.size = size;
  record.crc32 = util::crc32(data, size);

  file_name2irecord.emplace(std::make_pair(file_name, records.size()));
  records.emplace_back(std::move(record));
}
void ZipArchive::to_bytes(std::vector<uint8_t>& out) const {
  ZipArchiver archiver(records);
  archiver.archive();
  out = archiver.stream.take();
}


} // namespace zip
} // namespace liong
