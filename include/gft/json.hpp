// JSON serialization/deserialization.
// @PENGUINLIONG
#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>

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

// JSON array builder.
struct JsonArray {
  std::vector<JsonValue> inner;

  inline JsonArray() : inner() {}
  JsonArray(std::vector<JsonValue>&& b) : inner(std::move(b)) {}
  JsonArray(std::initializer_list<JsonValue>&& elems);
};
// JSON object builder.
struct JsonObject {
  std::map<std::string, JsonValue> inner;

  inline JsonObject() : inner() {}
  JsonObject(std::map<std::string, JsonValue>&& b) : inner(std::move(b)) {}
  JsonObject(
    std::initializer_list<std::pair<const std::string, JsonValue>>&& entries
  );
};

// Represent a abstract value in JSON representation.
struct JsonValue {
  JsonType ty;
  bool b;
  double num;
  std::string str;
  JsonObject obj;
  JsonArray arr;

  inline JsonValue() : ty(L_JSON_NULL) {}
  inline JsonValue(nullptr_t) : ty(L_JSON_NULL) {}
  inline JsonValue(bool b) : ty(L_JSON_BOOLEAN), b(b) {}
  inline JsonValue(double num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(float num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(char num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(signed char num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(unsigned char num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(short num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(unsigned short num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(int num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(unsigned int num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(long num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(unsigned long num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(long long num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(unsigned long long num) : ty(L_JSON_NUMBER), num(num) {}
  inline JsonValue(const char* str) : ty(L_JSON_STRING), str(str) {}
  inline JsonValue(const std::string& str) : ty(L_JSON_STRING), str(str) {}
  inline JsonValue(std::string&& str) :
    ty(L_JSON_STRING),
    str(std::forward<std::string>(str)) {}
  JsonValue(JsonObject&& obj);
  JsonValue(JsonArray&& arr);

  inline JsonValue& operator[](const char* key) {
    if (!is_obj()) { throw JsonException("value is not an object"); }
    return obj.inner.at(key);
  }
  inline const JsonValue& operator[](const char* key) const {
    if (!is_obj()) { throw JsonException("value is not an object"); }
    return obj.inner.at(key);
  }
  inline JsonValue& operator[](const std::string& key) {
    if (!is_obj()) { throw JsonException("value is not an object"); }
    return obj.inner.at(key);
  }
  inline const JsonValue& operator[](const std::string& key) const {
    if (!is_obj()) { throw JsonException("value is not an object"); }
    return obj.inner.at(key);
  }
  inline JsonValue& operator[](size_t i) {
    if (!is_arr()) { throw JsonException("value is not an array"); }
    return arr.inner.at(i);
  }
  inline const JsonValue& operator[](size_t i) const {
    if (!is_arr()) { throw JsonException("value is not an array"); }
    return arr.inner.at(i);
  }
  inline operator bool() const {
    if (!is_bool()) { throw JsonException("value is not a bool"); }
    return b;
  }
  inline operator double() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return num;
  }
  inline operator float() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (float)num;
  }
  inline operator char() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (char)num;
  }
  inline operator signed char() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (signed char)num;
  }
  inline operator unsigned char() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (unsigned char)num;
  }
  inline operator short() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (short)num;
  }
  inline operator unsigned short() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (unsigned short)num;
  }
  inline operator int() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (int)num;
  }
  inline operator unsigned int() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (unsigned int)num;
  }
  inline operator long() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (long)num;
  }
  inline operator unsigned long() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (unsigned long)num;
  }
  inline operator long long() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (long long)num;
  }
  inline operator unsigned long long() const {
    if (!is_num()) { throw JsonException("value is not a number"); }
    return (unsigned long long)num;
  }
  inline operator const std::string& () const {
    if (!is_str()) { throw JsonException("value is not a string"); }
    return str;
  }
  inline operator const JsonArray& () const {
    if (!is_arr()) { throw JsonException("value is not an array"); }
    return arr;
  }
  inline operator const JsonObject& () const {
    if (!is_obj()) { throw JsonException("value is not an object"); }
    return obj;
  }

  inline bool is_null() const { return ty == L_JSON_NULL; }
  inline bool is_bool() const { return ty == L_JSON_BOOLEAN; }
  inline bool is_num() const { return ty == L_JSON_NUMBER; }
  inline bool is_str() const { return ty == L_JSON_STRING; }
  inline bool is_obj() const { return ty == L_JSON_OBJECT; }
  inline bool is_arr() const { return ty == L_JSON_ARRAY; }

  inline size_t size() const {
    if (is_obj()) {
      return obj.inner.size();
    } else if (is_arr()) {
      return arr.inner.size();
    } else {
      throw JsonException("only object and array can have size");
    }
  }
  inline JsonElementEnumerator elems() const {
    return JsonElementEnumerator(arr.inner);
  }
  inline JsonFieldEnumerator fields() const {
    return JsonFieldEnumerator(obj.inner);
  }
};

// Parse JSON literal into and `JsonValue` object. If the JSON is invalid or
// unsupported, `JsonException` will be raised.
JsonValue parse(const std::string& json_lit);
// Returns true when JSON parsing successfully finished and parsed value is
// returned via `out`. Otherwise, false is returned and out contains incomplete
// result.
bool try_parse(const std::string& json_lit, JsonValue& out);

std::string print(const JsonValue& json);

} // namespace json
} // namespace liong
