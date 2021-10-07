// # Index and Size Processing Utilities
// @PENGUINLIONG

namespace liong {

constexpr size_t div_down(size_t x, size_t align) {
  return x / align;
}
constexpr size_t div_up(size_t x, size_t align) {
  return div_down(x + (align - 1), align);
}
constexpr size_t align_down(size_t x, size_t align) {
  return div_down(x, align) * align;
}
constexpr size_t align_up(size_t x, size_t align) {
  return div_up(x, align) * align;
}

constexpr void push_idx(size_t& aggr_idx, size_t i, size_t dim_size) {
  aggr_idx = aggr_idx * dim_size + i;
}
constexpr size_t pop_idx(size_t& aggr_idx, size_t dim_size) {
  size_t out = aggr_idx % dim_size;
  aggr_idx /= dim_size;
  return out;
}

} // namespace liong
