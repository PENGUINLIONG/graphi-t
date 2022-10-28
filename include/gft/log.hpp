// Logging infrastructure.
// @PENGUINLIONG
#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include "gft/util.hpp"


namespace liong {

namespace log {
// Logging infrastructure.

enum class LogLevel {
  L_LOG_LEVEL_DEBUG = 0,
  L_LOG_LEVEL_INFO = 1,
  L_LOG_LEVEL_WARNING = 2,
  L_LOG_LEVEL_ERROR = 3,
};

typedef void (*LogCallback)(LogLevel lv, const std::string& msg);

void set_log_callback(LogCallback cb);
void set_log_filter_level(LogLevel lv);
template<typename ... TArgs>
void log(LogLevel lv, const TArgs& ... msg) {
  if (detail::log_callback != nullptr && lv >= detail::filter_lv) {
    std::string indent(detail::indent, ' ');
    detail::log_callback(lv, util::format(indent, msg...));
  }
}

void push_indent();
void pop_indent();

template<typename ... TArgs>
inline void debug(const TArgs& ... msg) {
#ifdef L_MIN_LOG_LEVEL
  if constexpr ((uint32_t)LogLevel::L_LOG_LEVEL_DEBUG >= L_MIN_LOG_LEVEL)
#endif // L_MIN_LOG_LEVEL
  log(LogLevel::L_LOG_LEVEL_DEBUG, msg...);
}
template<typename ... TArgs>
inline void info(const TArgs& ... msg) {
#ifdef L_MIN_LOG_LEVEL
  if constexpr ((uint32_t)LogLevel::L_LOG_LEVEL_INFO >= L_MIN_LOG_LEVEL)
#endif // L_MIN_LOG_LEVEL
  log(LogLevel::L_LOG_LEVEL_INFO, msg...);
}
template<typename ... TArgs>
inline void warn(const TArgs& ... msg) {
#ifdef L_MIN_LOG_LEVEL
  if constexpr ((uint32_t)LogLevel::L_LOG_LEVEL_WARNING >= L_MIN_LOG_LEVEL)
#endif // L_MIN_LOG_LEVEL
  log(LogLevel::L_LOG_LEVEL_WARNING, msg...);
}
template<typename ... TArgs>
inline void error(const TArgs& ... msg) {
#ifdef L_MIN_LOG_LEVEL
  if constexpr ((uint32_t)LogLevel::L_LOG_LEVEL_ERROR >= L_MIN_LOG_LEVEL)
#endif // L_MIN_LOG_LEVEL
  log(LogLevel::L_LOG_LEVEL_ERROR, msg...);
}
}

} // namespace liong
