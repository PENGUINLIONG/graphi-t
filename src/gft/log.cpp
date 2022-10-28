#include "gft/log.hpp"

namespace liong {

namespace log {

namespace {

void l_default_log_cb_impl__(liong::log::LogLevel lv, const std::string& msg) {
  using liong::log::LogLevel;
  switch (lv) {
  case LogLevel::L_LOG_LEVEL_DEBUG:
    printf("[\x1b[90mDEBUG\x1B[0m] %s\n", msg.c_str());
    break;
  case LogLevel::L_LOG_LEVEL_INFO:
    printf("[\x1B[32mINFO\x1B[0m] %s\n", msg.c_str());
    break;
  case LogLevel::L_LOG_LEVEL_WARNING:
    printf("[\x1B[33mWARN\x1B[0m] %s\n", msg.c_str());
    break;
  case LogLevel::L_LOG_LEVEL_ERROR:
    printf("[\x1B[31mERROR\x1B[0m] %s\n", msg.c_str());
    break;
  }
}

LogCallback l_default_log_cb__ = &l_default_log_cb_impl__;
LogLevel l_filter_lv__;
uint32_t l_log_indent__;

} // namespace


void set_log_callback(LogCallback cb) {
  l_default_log_cb__ = cb;
}
void set_log_filter_level(LogLevel lv) {
  l_filter_lv__ = lv;
}

void push_indent() {
  l_log_indent__ += 4;
}
void pop_indent() {
  l_log_indent__ -= 4;
}

} // namespace log

} // namespace liong
