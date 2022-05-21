// # 3D Mesh utilities
// @PENGUINLIONG
#include <set>
#include "gft/vmath.hpp"
#include "gft/mesh.hpp"
#include "gft/assert.hpp"
#include "gft/log.hpp"

namespace liong {
namespace mesh {

using namespace vmath;

enum ObjTokenType {
  L_OBJ_TOKEN_TYPE_NEWLINE,
  L_OBJ_TOKEN_TYPE_VERB,
  L_OBJ_TOKEN_TYPE_TEXT,
  L_OBJ_TOKEN_TYPE_INTEGER,
  L_OBJ_TOKEN_TYPE_NUMBER,
  L_OBJ_TOKEN_TYPE_SLASH,
  L_OBJ_TOKEN_TYPE_END,
};
struct ObjToken {
  ObjTokenType token_ty;
  std::string token_word;
  uint32_t token_integer;
  float token_number;
};

struct ObjTokenizer {
  const char* pos;
  const char* end;
  bool success;
  ObjToken token;

  ObjTokenizer(const char* beg, const char* end) :
    pos(beg), end(end), success(true), token()
  {
    next();
  }

  bool try_tokenize(ObjToken& token) {
    bool is_in_token = false;
    bool is_last_token_newline = token.token_ty == L_OBJ_TOKEN_TYPE_NEWLINE;
    ObjTokenType token_ty;
    std::string buf {};

    for (;;) {
      if (is_in_token) {
        // It's already been parsing an multi-char token, can be a verb,
        // interger or a real number.
        bool should_break_token = pos == end;
        if (pos != end) {
          const char c = *pos;
          should_break_token |= c == ' ' || c == '\t' || c == '\r' ||
            c == '\n' || c == '/' || c == '#';
        }

        if (should_break_token) {
          // Punctuation breaks tokens, but don't step forward `pos` here, the
          // newlines need to be tokenized later.
          token.token_ty = token_ty;
          switch (token_ty) {
            case L_OBJ_TOKEN_TYPE_VERB:
            case L_OBJ_TOKEN_TYPE_TEXT:
              token.token_word = std::move(buf);
              break;
            case L_OBJ_TOKEN_TYPE_INTEGER:
              token.token_integer = std::atoi(buf.c_str());
              break;
            case L_OBJ_TOKEN_TYPE_NUMBER:
              token.token_number = std::atof(buf.c_str());
              break;
            default: unreachable();
          }
          is_in_token = false;
          return true;
        }

        const char c = *pos;

        switch (token_ty) {
        case L_OBJ_TOKEN_TYPE_INTEGER:
          if (c == '.') {
            // Found a fraction point so promote the integer into a floating-
            // point number.
            token_ty = L_OBJ_TOKEN_TYPE_NUMBER;
            break;
          } else if (c >= '0' && c <= '9') {
            break;
          } else if (c != '\0' && c < 128) {
            token_ty = L_OBJ_TOKEN_TYPE_TEXT;
            break;
          } else {
            return false;
          }
        case L_OBJ_TOKEN_TYPE_NUMBER:
          if (c >= '0' && c <= '9') {
          } else if (c != '\0' && c < 128) {
            token_ty = L_OBJ_TOKEN_TYPE_TEXT;
            break;
          } else {
            return false;
          }
          break;
        case L_OBJ_TOKEN_TYPE_VERB:
          if (c >= 'a' && c <= 'z') {
            break;
          } else if (c != '\0' && c < 128) {
            token_ty = L_OBJ_TOKEN_TYPE_TEXT;
            break;
          } else {
            return false;
          }
        case L_OBJ_TOKEN_TYPE_TEXT:
          if (c != '\0' && c < 128) {
            break;
          } else {
            return false;
          }
        default: unreachable();
        }
        buf.push_back(c);

      } else {
        // It's not parsing a token.
        if (pos == end) {
          // End of input.
          token.token_ty = L_OBJ_TOKEN_TYPE_END;
          return true;
        }

        char c = *pos;

        if (c == '\n') {
          // Newlines.
          ++pos;
          token.token_ty = L_OBJ_TOKEN_TYPE_NEWLINE;
          return true;
        }
        if (c == '#') {
          // Skip comments.
          while (pos != end && *(++pos) != '\n');
          token.token_ty = L_OBJ_TOKEN_TYPE_NEWLINE;
          return true;
        }
        if (c == '/') {
          ++pos;
          token.token_ty = L_OBJ_TOKEN_TYPE_SLASH;
          return true;
        }

        // Ignore white spaces.
        while (c == ' ' || c == '\t' || c == '\r') { ++pos; c = *pos; }

        if (c == '-' || (c >= '0' && c <= '9')) {
          token_ty = L_OBJ_TOKEN_TYPE_INTEGER;
          buf.push_back(c);
          is_in_token = true;
        } else if (is_last_token_newline && (c >= 'a' && c <= 'z')) {
          token_ty = L_OBJ_TOKEN_TYPE_VERB;
          buf.push_back(c);
          is_in_token = true;
        } else if (c != '\0' && c < 128) {
          // Any other visible characters.
          token_ty = L_OBJ_TOKEN_TYPE_TEXT;
          buf.push_back(c);
          is_in_token = true;
        } else {
          unreachable();
        }
      }
      ++pos;

    }
  }

  bool try_next() {
    return try_tokenize(token);
  }
  void next() {
    success &= try_next();
  }

  bool try_verb(std::string& out) {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_VERB) {
      out = token.token_word;
      next();
      return true;
    }
    return false;
  }
  void verb(std::string& out) {
    success &= try_verb(out);
  }
  bool try_integer(uint32_t& out) {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_INTEGER) {
      out = token.token_integer;
      next();
      return true;
    }
    return false;
  }
  void integer(uint32_t& out) {
    success &= try_integer(out);
  }
  bool try_number(float& out) {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_INTEGER) {
      out = token.token_integer;
      next(); 
      return true;
    } else if (token.token_ty == L_OBJ_TOKEN_TYPE_NUMBER) {
      out = token.token_number;
      next();
      return true;
    }
    return false;
  }
  void number(float& out) {
    success &= try_number(out);
  }
  bool try_slash() {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_SLASH) {
      next();
      return true;
    }
    return false;
  }
  bool try_newline() {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_NEWLINE) {
      next();
      return true;
    }
    return false;
  }
  void newline() {
    success &= try_newline();
  }
  bool try_end() {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_END) {
      next();
      return true;
    }
    return false;
  }
};

struct ObjParser {
  ObjTokenizer tokenizer;
  ObjParser(const char* beg, const char* end) : tokenizer(beg, end) {}

  bool try_parse(Mesh& out) {
    Mesh mesh {};
    std::set<std::string> unknown_verbs;

    std::vector<vmath::float3> poses;
    std::vector<vmath::float2> uvs;
    std::vector<vmath::float3> norms;
    uint32_t counter = 0;

    while (tokenizer.success) {
      std::string verb;
      if (tokenizer.try_verb(verb)) {
        if (tokenizer.token.token_word == "v") {
          vmath::float3 pos{};
          float w;
          tokenizer.number(pos.x);
          tokenizer.number(pos.y);
          tokenizer.number(pos.z);
          tokenizer.try_number(w);
          tokenizer.newline();
          poses.emplace_back(std::move(pos));
          continue;
        } else if (tokenizer.token.token_word == "vt") {
          vmath::float2 uv{};
          float w;
          tokenizer.number(uv.x);
          tokenizer.try_number(uv.y);
          tokenizer.try_number(w);
          tokenizer.newline();
          uvs.emplace_back(std::move(uv));
          continue;
        } else if (tokenizer.token.token_word == "vn") {
          vmath::float3 norm{};
          tokenizer.number(norm.x);
          tokenizer.number(norm.y);
          tokenizer.number(norm.z);
          tokenizer.newline();
          norms.emplace_back(std::move(norm));
          continue;
        } else if (tokenizer.token.token_word == "f") {
          uint32_t ipos;
          uint32_t iuv;
          uint32_t inorm;
          for (uint32_t i = 0; i < 3; ++i) {
            tokenizer.integer(ipos);
            mesh.poses.emplace_back(poses.at(ipos - 1));
            if (tokenizer.try_slash()) {
              if (tokenizer.try_integer(iuv)) {
                mesh.uvs.emplace_back(uvs.at(iuv - 1));
              }
            }
            if (tokenizer.try_slash()) {
              if (tokenizer.try_integer(inorm)) {
                mesh.norms.emplace_back(norms.at(inorm - 1));
              }
            }
          }
          tokenizer.newline();
          continue;
        }

        // We don't know this verb, report the case and skip this line.
        {
          if (unknown_verbs.find(verb) == unknown_verbs.end()) {
            unknown_verbs.emplace(verb);
            log::warn("unknown obj verb '", verb, "' is ignored");
          }
          while (tokenizer.success) {
            if (tokenizer.try_newline() || tokenizer.try_end()) {
              break;
            } else {
              tokenizer.next();
            }
          }
        }
      }

      if (tokenizer.try_newline()) {
        // Ignore empty newlines.
        continue;
      } else if (tokenizer.try_end()) {
        if (mesh.uvs.size() == 0) {
          log::warn("uv data is not available, filled with zeroes instead");
          mesh.uvs.resize(mesh.poses.size());
        } else if (mesh.uvs.size() != mesh.poses.size()) {
          log::warn("uv count mismatches position count; treated as error");
          return false;
        }
        if (mesh.norms.size() == 0) {
          log::warn("normal data is not available, filled with zeroes instead");
          mesh.norms.resize(mesh.poses.size());
        } else if (mesh.norms.size() != mesh.poses.size()) {
          log::warn("normal count mismatches position count; treated as error");
          return false;
        }
        out = std::move(mesh);
        return true;
      }
    }
    return false;
  }
};

bool try_parse_obj(const std::string& obj, Mesh& mesh) {
  ObjParser parser(obj.data(), obj.data() + obj.size());
  return parser.try_parse(mesh);
}
Mesh load_obj(const char* path) {
  auto txt = util::load_text(path);
  Mesh mesh{};
  assert(try_parse_obj(txt, mesh));
  return mesh;
}



struct UniqueVertex {
  vmath::float3 pos;
  vmath::float2 uv;
  vmath::float3 norm;

  friend bool operator<(const UniqueVertex& a, const UniqueVertex& b) {
    return std::memcmp(&a, &b, sizeof(UniqueVertex)) < 0;
  }
};
IndexedMesh mesh2idxmesh(const Mesh& mesh) {
  IndexedMesh out{};
  std::map<UniqueVertex, uint32_t> vert2idx;

  for (size_t i = 0; i < mesh.poses.size(); ++i) {
    UniqueVertex vert;
    vert.pos = mesh.poses.at(i);
    vert.uv = mesh.uvs.at(i);
    vert.norm = mesh.norms.at(i);

    uint32_t idx;
    auto it = vert2idx.find(vert);
    if (it == vert2idx.end()) {
      idx = vert2idx.size();
      out.mesh.poses.emplace_back(vert.pos);
      out.mesh.uvs.emplace_back(vert.uv);
      out.mesh.norms.emplace_back(vert.norm);
      vert2idx.insert(it, std::make_pair(std::move(vert), idx));
    } else {
      idx = it->second;
    }
    out.idxs.emplace_back(idx);
  }

  return out;
}

} // namespace mesh
} // namespace liong
