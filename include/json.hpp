// JSON serialization/deserialization.
// @PENGUINLIONG
#pragma once
#include <string>
#include <vector>
#include <map>

namespace liong {
namespace json {

// Any error occured during JSON serialization/deserialization.
class JsonException : public std::exception {
private:
  std::string msg;
public:
  JsonException(const char* msg);
  const char* what() const noexcept override;
};

// Type of JSON value.
enum JsonType {
  L_JSON_NULL,
  L_JSON_BOOLEAN,
  L_JSON_NUMBER,
  L_JSON_STRING,
  L_JSON_OBJECT,
  L_JSON_ARRAY,
};

struct JsonValue;

class JsonElementEnumerator {
  std::vector<JsonValue>::const_iterator beg_, end_;
public:
  JsonElementEnumerator(const std::vector<JsonValue>& arr) :
    beg_(arr.cbegin()), end_(arr.cend()) {}

  std::vector<JsonValue>::const_iterator begin() const {
    return beg_;
  }
  std::vector<JsonValue>::const_iterator end() const {
    return end_;
  }
};

class JsonFieldEnumerator {
  std::map<std::string, JsonValue>::const_iterator beg_, end_;
public:
  JsonFieldEnumerator(const std::map<std::string, JsonValue>& obj) :
    beg_(obj.cbegin()), end_(obj.cend()) {}

  std::map<std::string, JsonValue>::const_iterator begin() const {
    return beg_;
  }
  std::map<std::string, JsonValue>::const_iterator end() const {
    return end_;
  }
};

// Represent a abstract value in JSON representation.
struct JsonValue {
  JsonType ty;
  bool b;
  double num;
  std::string str;
  std::map<std::string, JsonValue> obj;
  std::vector<JsonValue> arr;

  inline JsonValue& operator[](const std::string& key) {
    if (!is_obj()) { throw JsonException("value is not an object"); }
    return obj.at(key);
  }
  inline const JsonValue& operator[](const std::string& key) const {
    if (!is_obj()) { throw JsonException("value is not an object"); }
    return obj.at(key);
  }
  inline JsonValue& operator[](size_t i) {
    if (!is_arr()) { throw JsonException("value is not an array"); }
    return arr.at(i);
  }
  inline const JsonValue& operator[](size_t i) const {
    if (!is_arr()) { throw JsonException("value is not an array"); }
    return arr.at(i);
  }
  explicit inline operator bool() const {
    if (!is_bool()) { throw JsonException("value is not a bool"); }
    return b;
  }
  explicit inline operator double() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return num;
  }
  explicit inline operator float() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (float)num;
  }
  explicit inline operator int() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (int)num;
  }
  explicit inline operator unsigned int() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (unsigned int)num;
  }
  explicit inline operator long() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (long)num;
  }
  explicit inline operator unsigned long() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (unsigned long)num;
  }
  explicit inline operator std::string() const {
    if (!is_str()) { throw JsonException("value is not a string"); }
    return str;
  }

  inline bool is_null() const { return ty == L_JSON_NULL; }
  inline bool is_bool() const { return ty == L_JSON_BOOLEAN; }
  inline bool is_num() const { return ty == L_JSON_NUMBER; }
  inline bool is_str() const { return ty == L_JSON_STRING; }
  inline bool is_obj() const { return ty == L_JSON_OBJECT; }
  inline bool is_arr() const { return ty == L_JSON_ARRAY; }

  inline size_t size() const {
    if (is_obj()) {
      return obj.size();
    } else if (is_arr()) {
      return arr.size();
    } else {
      throw JsonException("only object and array can have size");
    }
  }
  inline JsonElementEnumerator elems() const {
    return JsonElementEnumerator(arr);
  }
  inline JsonFieldEnumerator fields() const {
    return JsonFieldEnumerator(obj);
  }
};

// Parse JSON literal into and `JsonValue` object. If the JSON is invalid or
// unsupported, `JsonException` will be raised.
JsonValue parse(const std::string& json_lit);
// Returns true when JSON parsing successfully finished and parsed value is
// returned via `out`. Otherwise, false is returned and out contains incomplete
// result.
bool try_parse(const std::string& json_lit, JsonValue& out);

} // namespace json
} // namespace liong
