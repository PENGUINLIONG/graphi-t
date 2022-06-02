// # 3D Mesh utilities
// @PENGUINLIONG
#include <cstdint>
#include <set>
#include <initializer_list>
#include "glm/glm.hpp"
#include "gft/mesh.hpp"
#include "gft/assert.hpp"
#include "gft/log.hpp"

namespace liong {
namespace mesh {

using namespace geom;

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

        if (!should_break_token) {
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

        if (c == '\r' || c == '\n') {
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
        while (c == ' ' || c == '\t') {
          ++pos; c = *pos;
        }

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
  bool try_end() {
    if (pos == end) {
      return true;
    }
    if (token.token_ty == L_OBJ_TOKEN_TYPE_END) {
      next();
      return true;
    }
    return false;
  }
};

struct ObjParser {
  ObjTokenizer tokenizer;
  std::string verb;
  ObjParser(const char* beg, const char* end) : tokenizer(beg, end) {}

  bool try_parse(Mesh& out) {
    Mesh mesh {};
    std::set<std::string> unknown_verbs;

    std::vector<glm::vec3> poses;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> norms;
    uint32_t counter = 0;

    while (tokenizer.success) {
      if (tokenizer.try_verb(verb)) {
        if (tokenizer.token.token_word == "v") {
          glm::vec3 pos{};
          float w;
          tokenizer.number(pos.x);
          tokenizer.number(pos.y);
          tokenizer.number(pos.z);
          tokenizer.try_number(w);
          tokenizer.try_newline();
          poses.emplace_back(std::move(pos));
          continue;
        } else if (tokenizer.token.token_word == "vt") {
          glm::vec2 uv{};
          float w;
          tokenizer.number(uv.x);
          tokenizer.try_number(uv.y);
          tokenizer.try_number(w);
          tokenizer.try_newline();
          uvs.emplace_back(std::move(uv));
          continue;
        } else if (tokenizer.token.token_word == "vn") {
          glm::vec3 norm{};
          tokenizer.number(norm.x);
          tokenizer.number(norm.y);
          tokenizer.number(norm.z);
          tokenizer.try_newline();
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
          tokenizer.try_newline();
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



Mesh Mesh::from_tris(const Triangle* tris, size_t ntri) {
  Mesh out {};
  out.poses.reserve(ntri * 3);
  out.uvs.resize(ntri * 3);
  out.norms.resize(ntri * 3);

  for (size_t i = 0; i < ntri; ++i) {
    const Triangle& tri = tris[i];
    out.poses.emplace_back(tri.a);
    out.poses.emplace_back(tri.b);
    out.poses.emplace_back(tri.c);
  }
  return out;
}
std::vector<Triangle> Mesh::to_tris() const {
  std::vector<Triangle> out {};
  size_t ntri = poses.size() / 3;
  assert(ntri * 3 == poses.size());
  out.reserve(ntri);
  for (size_t i = 0; i < poses.size(); i += 3) {
    Triangle tri {
      poses.at(i),
      poses.at(i + 1),
      poses.at(i + 2),
    };
    out.emplace_back(std::move(tri));
  }
  return out;
}


Aabb Mesh::aabb() const {
  return Aabb::from_points(poses.data(), poses.size());
}



struct UniqueVertex {
  glm::vec3 pos;
  glm::vec2 uv;
  glm::vec3 norm;

  friend bool operator<(const UniqueVertex& a, const UniqueVertex& b) {
    return std::memcmp(&a, &b, sizeof(UniqueVertex)) < 0;
  }
};
IndexedMesh IndexedMesh::from_mesh(const Mesh& mesh) {
  IndexedMesh out{};
  std::map<UniqueVertex, uint32_t> vert2idx;

  uint32_t ntri = mesh.poses.size() / 3;
  if (ntri * 3 != mesh.poses.size()) {
    log::warn("mesh vertex number is not aligned to 3; trailing vertices are "
      "ignored because they don't form an actual triangle");
  }

  for (uint32_t i = 0; i < ntri; ++i) {
    uint32_t idxs[3];
    for (uint32_t j = 0; j < 3; ++j) {
      uint32_t ivert = i * 3 + j;
      UniqueVertex vert;
      vert.pos = mesh.poses.at(ivert);
      vert.uv = mesh.uvs.at(ivert);
      vert.norm = mesh.norms.at(ivert);

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
      idxs[j] = idx;
    }
    out.idxs.emplace_back(glm::uvec3 { idxs[0], idxs[1], idxs[2] });
  }

  return out;
}

struct Binner {
  Aabb aabb;
  glm::uvec3 grid_res;
  // `aabb` range divided by `grid_res` except for the exactly `min` vlaues.
  std::vector<float> grid_lines_x;
  std::vector<float> grid_lines_y;
  std::vector<float> grid_lines_z;
  std::vector<Bin> bins;
  uint32_t counter;

  static std::vector<float> make_grid_lines(float min, float max, uint32_t n) {
    float range = max - min;
    std::vector<float> out;
    for (uint32_t i = 1; i < n; ++i) {
      out.emplace_back((i / float(n)) * range + min);
    }
    out.emplace_back(max);
    return out;
  }

  Binner(const Aabb& aabb, const glm::uvec3& grid_res) :
    aabb(aabb),
    grid_res(grid_res),
    grid_lines_x(make_grid_lines(aabb.min.x, aabb.max.x, grid_res.x)),
    grid_lines_y(make_grid_lines(aabb.min.y, aabb.max.y, grid_res.y)),
    grid_lines_z(make_grid_lines(aabb.min.z, aabb.max.z, grid_res.z)),
    bins()
  {
    bins.reserve(grid_res.x * grid_res.y * grid_res.z);
    for (uint32_t z = 0; z < grid_res.z; ++z) {
      float z_min = z == 0 ? aabb.min.z : grid_lines_z.at(z - 1);
      float z_max = grid_lines_z.at(z);
      for (uint32_t y = 0; y < grid_res.y; ++y) {
        float y_min = y == 0 ? aabb.min.y : grid_lines_y.at(y - 1);
        float y_max = grid_lines_y.at(y);
        for (uint32_t x = 0; x < grid_res.x; ++x) {
          float x_min = x == 0 ? aabb.min.x : grid_lines_x.at(x - 1);
          float x_max = grid_lines_x.at(x);

          glm::vec3 min(x_min, y_min, z_min);
          glm::vec3 max(x_max, y_max, z_max);
          Aabb aabb2 { min, max };

          Bin bin { aabb2, {} };
          bins.emplace_back(bin);
        }
      }
    }
  }

  size_t get_ibin(const std::vector<float>& grid_lines, float x) {
    size_t i = 1;
    // The loop breaks when `x` is less than `grid_lines` so `i` ended at any
    // index of the left-close and right-open interval that contains the point.
    // But also note that if the loop runs out of range, i.e., `x >= aabb.max`
    // the index of the farthest bin is naturally assigned to `i`. Given that
    // non-intersecting triangles are filetered in `bin` any point enclosed
    // by `aabb` can be uniquely assigned to a bin at boundaries.
    for (; i < grid_lines.size(); ++i) {
      if (x < grid_lines.at(i)) { break; }
    }
    return i - 1;
  }

  bool bin(const Aabb& aabb, size_t& iprim) {
    if (!intersect_aabb(this->aabb, aabb)) {
      // The triangle's AABB is not intersecting with the current binner space.
      // So simply ignore this triangle.
      return false;
    }

    size_t imin_x = get_ibin(grid_lines_x, aabb.min.x);
    size_t imax_x = get_ibin(grid_lines_x, aabb.max.x);
    size_t imin_y = get_ibin(grid_lines_y, aabb.min.y);
    size_t imax_y = get_ibin(grid_lines_y, aabb.max.y);
    size_t imin_z = get_ibin(grid_lines_z, aabb.min.z);
    size_t imax_z = get_ibin(grid_lines_z, aabb.max.z);
    // TODO: (penguinliong) Consider the case of a very narrow triangle placed
    // on the diagonal of the bins looped here. There is a significant room of
    // optimization for large triangles.
    for (size_t z = imin_z; z <= imax_z; ++z) {
      for (size_t y = imin_y; y <= imax_y; ++y) {
        for (size_t x = imin_x; x <= imax_x; ++x) {
          size_t i = ((z * grid_res.y + y) * grid_res.x) + x;
          bins.at(i).iprims.emplace_back(counter);
        }
      }
    }
    ++counter;
    return true;
  }

  BinGrid into_grid() {
    BinGrid grid {
      std::move(grid_lines_x),
      std::move(grid_lines_y),
      std::move(grid_lines_z),
      std::move(bins),
    };
    return grid;
  }
};

BinGrid bin_point_cloud(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const PointCloud& point_cloud
) {
  Binner binner(aabb, grid_res);
  for (const auto& point : point_cloud.poses) {
    Aabb aabb2 { point };
    size_t _;
    binner.bin(aabb2, _);
  }
  return binner.into_grid();
}

BinGrid bin_mesh(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const Mesh& mesh
) {
  Binner binner(aabb, grid_res);
  for (size_t i = 0; i < mesh.poses.size(); i += 3) {
    glm::vec3 points[3] {
      mesh.poses.at(i),
      mesh.poses.at(i + 2),
      mesh.poses.at(i + 1),
    };
    Aabb aabb2 = Aabb::from_points(points, 3);
    size_t _;
    binner.bin(aabb2, _);
  }
  return binner.into_grid();
}
BinGrid bin_mesh(
  const glm::vec3& grid_interval,
  const Mesh& mesh
) {
  Aabb aabb = mesh.aabb();
  glm::uvec3 grid_res = glm::ceil(aabb.size() / grid_interval);
  aabb = Aabb::from_center_size(
    aabb.center(),
    glm::vec3(grid_res) * grid_interval);
  return bin_mesh(aabb, grid_res, mesh);
}

BinGrid bin_idxmesh(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const IndexedMesh& idxmesh
) {
  Binner binner(aabb, grid_res);
  for (const auto& idx : idxmesh.idxs) {
    glm::vec3 points[3] {
      idxmesh.mesh.poses.at(idx.x),
      idxmesh.mesh.poses.at(idx.y),
      idxmesh.mesh.poses.at(idx.z),
    };
    Aabb aabb2 = Aabb::from_points(points, 3);
    size_t _;
    binner.bin(aabb2, _);
  }
  return binner.into_grid();
}
BinGrid bin_idxmesh(
  const glm::vec3& grid_interval,
  const IndexedMesh& idxmesh
) {
  Aabb aabb = idxmesh.aabb();
  glm::uvec3 grid_res = glm::ceil(aabb.size() / grid_interval);
  aabb = Aabb::from_center_size(
    aabb.center(),
    glm::vec3(grid_res) * grid_interval);
  return bin_idxmesh(aabb, grid_res, idxmesh);
}


void compact_mesh(BinGrid& grid) {
  for (Bin& bin : grid.bins) {
    std::vector<uint32_t> idxs;
  }
}

} // namespace mesh
} // namespace liong
