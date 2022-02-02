#include "gft/log.hpp"

namespace liong {

namespace log {

namespace detail {

decltype(log_callback) log_callback = nullptr;
LogLevel filter_lv;
uint32_t indent;

} // namespace detail



void set_log_callback(decltype(detail::log_callback) cb) {
  detail::log_callback = cb;
}
void set_log_filter_level(LogLevel lv) {
  detail::filter_lv = lv;
}

void push_indent() {
  detail::indent += 4;
}
void pop_indent() {
  detail::indent -= 4;
}

} // namespace log


} // namespace liong
