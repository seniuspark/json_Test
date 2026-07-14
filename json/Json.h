#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <stdexcept>
#include <sstream>

namespace json {

class Value;
using Array = std::vector<Value>;

// 객체는 삽입 순서를 보존하기 위해 (key, value) 쌍의 벡터로 구현한다.
using Object = std::vector<std::pair<std::string, Value>>;

enum class Type {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Value {
public:
    Value() : type_(Type::Null) {}
    Value(std::nullptr_t) : type_(Type::Null) {}
    Value(bool b) : type_(Type::Bool), bool_(b) {}
    Value(int i) : type_(Type::Number), num_(static_cast<double>(i)) {}
    Value(double d) : type_(Type::Number), num_(d) {}
    Value(const char* s) : type_(Type::String), str_(s) {}
    Value(std::string s) : type_(Type::String), str_(std::move(s)) {}
    Value(json::Array a) : type_(Type::Array), arr_(std::move(a)) {}
    Value(json::Object o) : type_(Type::Object), obj_(std::move(o)) {}

    static Value MakeArray() { Value v; v.type_ = Type::Array; return v; }
    static Value MakeObject() { Value v; v.type_ = Type::Object; return v; }

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }

    bool asBool() const { require(Type::Bool); return bool_; }
    double asDouble() const { require(Type::Number); return num_; }
    int asInt() const { require(Type::Number); return static_cast<int>(num_); }
    const std::string& asString() const { require(Type::String); return str_; }
    const json::Array& asArray() const { require(Type::Array); return arr_; }
    json::Array& asArray() { require(Type::Array); return arr_; }
    const json::Object& asObject() const { require(Type::Object); return obj_; }
    json::Object& asObject() { require(Type::Object); return obj_; }

    // 객체 접근: 키가 없으면 null Value 반환 (읽기 전용)
    const Value& operator[](const std::string& key) const;
    // 객체 접근: 키가 없으면 새로 추가 (쓰기용)
    Value& operator[](const std::string& key);

    // 배열 접근
    const Value& operator[](size_t index) const;
    Value& operator[](size_t index);

    bool contains(const std::string& key) const;

    void push_back(Value v);

    // JSON 문자열로 직렬화. indent < 0 이면 한 줄로 압축 출력.
    std::string dump(int indent = -1) const;

    // 문자열로부터 파싱
    static Value Parse(const std::string& text);

    // 파일에서 읽어 파싱
    static Value LoadFile(const std::string& path);

    // 파일로 저장
    void SaveFile(const std::string& path, int indent = 2) const;

private:
    void require(Type t) const {
        if (type_ != t) {
            throw std::runtime_error("json::Value: unexpected type access");
        }
    }

    void dumpImpl(std::ostringstream& out, int indent, int depth) const;

    Type type_;
    bool bool_ = false;
    double num_ = 0.0;
    std::string str_;
    json::Array arr_;
    json::Object obj_;
};

} // namespace json
