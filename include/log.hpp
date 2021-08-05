// Logging infrastructure.
// @PENGUINLIONG
#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include "util.hpp"


namespace liong {

namespace log {
// Logging infrastructure.

enum class LogLevel {
  L_LOG_LEVEL_DEBUG,
  L_LOG_LEVEL_INFO,
  L_LOG_LEVEL_WARNING,
  L_LOG_LEVEL_ERROR,
};

namespace detail {

extern void (*log_callback)(LogLevel lv, const std::string& msg);
extern LogLevel filter_lv;
extern uint32_t indent;

} // namespace detail

void set_log_callback(decltype(detail::log_callback) cb);
void set_log_filter_level(LogLevel lv);
template<typename ... TArgs>
void log(LogLevel lv, const TArgs& ... msg) {
  if (detail::log_callback != nullptr && lv >= detail::filter_lv) {
    std::string indent(detail::indent, ' ');
    detail::log_callback(lv, liong::util::format(indent, msg...));
  }
}

void push_indent();
void pop_indent();

template<typename ... TArgs>
inline void debug(const TArgs& ... msg) {
  log(LogLevel::L_LOG_LEVEL_DEBUG, msg...);
}
template<typename ... TArgs>
inline void info(const TArgs& ... msg) {
  log(LogLevel::L_LOG_LEVEL_INFO, msg...);
}
template<typename ... TArgs>
inline void warn(const TArgs& ... msg) {
  log(LogLevel::L_LOG_LEVEL_WARNING, msg...);
}
template<typename ... TArgs>
inline void error(const TArgs& ... msg) {
  log(LogLevel::L_LOG_LEVEL_ERROR, msg...);
}
}


} // namespace liong
