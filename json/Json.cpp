#include "Json.h"
#include <fstream>
#include <cctype>
#include <cmath>

namespace json {

namespace {

class Parser {
public:
    explicit Parser(const std::string& text) : s_(text), pos_(0), len_(text.size()) {}

    Value parse() {
        skipWhitespace();
        Value v = parseValue();
        skipWhitespace();
        if (pos_ != len_) {
            throw ParseError("파싱 후 여분의 문자가 남아있습니다 (위치: " + std::to_string(pos_) + ")");
        }
        return v;
    }

private:
    const std::string& s_;
    size_t pos_;
    size_t len_;

    char peek() const {
        if (pos_ >= len_) throw ParseError("예기치 않은 입력 끝");
        return s_[pos_];
    }

    char get() {
        if (pos_ >= len_) throw ParseError("예기치 않은 입력 끝");
        return s_[pos_++];
    }

    void skipWhitespace() {
        while (pos_ < len_) {
            char c = s_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    void expect(char c) {
        if (pos_ >= len_ || s_[pos_] != c) {
            throw ParseError(std::string("문자 '") + c + "'가 필요합니다 (위치: " + std::to_string(pos_) + ")");
        }
        ++pos_;
    }

    bool matchLiteral(const char* lit, size_t litLen) {
        if (pos_ + litLen > len_) return false;
        if (s_.compare(pos_, litLen, lit, litLen) == 0) {
            pos_ += litLen;
            return true;
        }
        return false;
    }

    Value parseValue() {
        skipWhitespace();
        if (pos_ >= len_) throw ParseError("예기치 않은 입력 끝");
        char c = peek();
        switch (c) {
            case '{': return parseObject();
            case '[': return parseArray();
            case '"': return Value(parseString());
            case 't':
                if (matchLiteral("true", 4)) return Value(true);
                throw ParseError("잘못된 리터럴 (위치: " + std::to_string(pos_) + ")");
            case 'f':
                if (matchLiteral("false", 5)) return Value(false);
                throw ParseError("잘못된 리터럴 (위치: " + std::to_string(pos_) + ")");
            case 'n':
                if (matchLiteral("null", 4)) return Value(nullptr);
                throw ParseError("잘못된 리터럴 (위치: " + std::to_string(pos_) + ")");
            default:
                if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                    return parseNumber();
                }
                throw ParseError("예상치 못한 문자 (위치: " + std::to_string(pos_) + ")");
        }
    }

    Value parseObject() {
        expect('{');
        Object obj;
        skipWhitespace();
        if (pos_ < len_ && peek() == '}') {
            ++pos_;
            return Value(std::move(obj));
        }
        while (true) {
            skipWhitespace();
            std::string key = parseString();
            skipWhitespace();
            expect(':');
            Value val = parseValue();
            obj.emplace_back(std::move(key), std::move(val));
            skipWhitespace();
            char c = get();
            if (c == ',') {
                continue;
            } else if (c == '}') {
                break;
            } else {
                throw ParseError("객체 파싱 중 ',' 또는 '}'가 필요합니다");
            }
        }
        return Value(std::move(obj));
    }

    Value parseArray() {
        expect('[');
        Array arr;
        skipWhitespace();
        if (pos_ < len_ && peek() == ']') {
            ++pos_;
            return Value(std::move(arr));
        }
        while (true) {
            Value val = parseValue();
            arr.push_back(std::move(val));
            skipWhitespace();
            char c = get();
            if (c == ',') {
                continue;
            } else if (c == ']') {
                break;
            } else {
                throw ParseError("배열 파싱 중 ',' 또는 ']'가 필요합니다");
            }
        }
        return Value(std::move(arr));
    }

    std::string parseString() {
        expect('"');
        std::string result;
        while (true) {
            char c = get();
            if (c == '"') break;
            if (c == '\\') {
                char esc = get();
                switch (esc) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u': {
                        unsigned int codepoint = parseHex4();
                        appendUtf8(result, codepoint);
                        break;
                    }
                    default:
                        throw ParseError("알 수 없는 이스케이프 시퀀스");
                }
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    unsigned int parseHex4() {
        if (pos_ + 4 > len_) throw ParseError("\\u 뒤에 4자리 16진수가 필요합니다");
        unsigned int value = 0;
        for (int i = 0; i < 4; ++i) {
            char c = s_[pos_++];
            value <<= 4;
            if (c >= '0' && c <= '9') value |= (c - '0');
            else if (c >= 'a' && c <= 'f') value |= (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= (c - 'A' + 10);
            else throw ParseError("잘못된 16진수 문자");
        }
        return value;
    }

    static void appendUtf8(std::string& out, unsigned int codepoint) {
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    Value parseNumber() {
        size_t start = pos_;
        if (pos_ < len_ && s_[pos_] == '-') ++pos_;
        while (pos_ < len_ && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        if (pos_ < len_ && s_[pos_] == '.') {
            ++pos_;
            while (pos_ < len_ && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        }
        if (pos_ < len_ && (s_[pos_] == 'e' || s_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < len_ && (s_[pos_] == '+' || s_[pos_] == '-')) ++pos_;
            while (pos_ < len_ && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        }
        std::string numStr = s_.substr(start, pos_ - start);
        if (numStr.empty() || numStr == "-") {
            throw ParseError("잘못된 숫자 형식");
        }
        return Value(std::stod(numStr));
    }
};

void escapeStringTo(std::ostringstream& out, const std::string& s) {
    out << '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out << buf;
                } else {
                    out << static_cast<char>(c);
                }
        }
    }
    out << '"';
}

void writeIndent(std::ostringstream& out, int indent, int depth) {
    if (indent >= 0) {
        out << '\n';
        out << std::string(static_cast<size_t>(indent) * depth, ' ');
    }
}

} // namespace

const Value& Value::operator[](const std::string& key) const {
    require(Type::Object);
    static const Value nullValue;
    for (const auto& kv : obj_) {
        if (kv.first == key) return kv.second;
    }
    return nullValue;
}

Value& Value::operator[](const std::string& key) {
    if (type_ == Type::Null) {
        type_ = Type::Object;
    }
    require(Type::Object);
    for (auto& kv : obj_) {
        if (kv.first == key) return kv.second;
    }
    obj_.emplace_back(key, Value());
    return obj_.back().second;
}

const Value& Value::operator[](size_t index) const {
    require(Type::Array);
    return arr_.at(index);
}

Value& Value::operator[](size_t index) {
    require(Type::Array);
    return arr_.at(index);
}

bool Value::contains(const std::string& key) const {
    require(Type::Object);
    for (const auto& kv : obj_) {
        if (kv.first == key) return true;
    }
    return false;
}

void Value::push_back(Value v) {
    if (type_ == Type::Null) {
        type_ = Type::Array;
    }
    require(Type::Array);
    arr_.push_back(std::move(v));
}

void Value::dumpImpl(std::ostringstream& out, int indent, int depth) const {
    switch (type_) {
        case Type::Null:
            out << "null";
            break;
        case Type::Bool:
            out << (bool_ ? "true" : "false");
            break;
        case Type::Number: {
            if (num_ == static_cast<long long>(num_)) {
                out << static_cast<long long>(num_);
            } else {
                out << num_;
            }
            break;
        }
        case Type::String:
            escapeStringTo(out, str_);
            break;
        case Type::Array: {
            if (arr_.empty()) {
                out << "[]";
                break;
            }
            out << '[';
            for (size_t i = 0; i < arr_.size(); ++i) {
                writeIndent(out, indent, depth + 1);
                arr_[i].dumpImpl(out, indent, depth + 1);
                if (i + 1 < arr_.size()) out << ',';
            }
            writeIndent(out, indent, depth);
            out << ']';
            break;
        }
        case Type::Object: {
            if (obj_.empty()) {
                out << "{}";
                break;
            }
            out << '{';
            for (size_t i = 0; i < obj_.size(); ++i) {
                writeIndent(out, indent, depth + 1);
                escapeStringTo(out, obj_[i].first);
                out << ':';
                if (indent >= 0) out << ' ';
                obj_[i].second.dumpImpl(out, indent, depth + 1);
                if (i + 1 < obj_.size()) out << ',';
            }
            writeIndent(out, indent, depth);
            out << '}';
            break;
        }
    }
}

std::string Value::dump(int indent) const {
    std::ostringstream out;
    dumpImpl(out, indent, 0);
    return out.str();
}

Value Value::Parse(const std::string& text) {
    Parser parser(text);
    return parser.parse();
}

Value Value::LoadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("파일을 열 수 없습니다: " + path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return Value::Parse(buffer.str());
}

void Value::SaveFile(const std::string& path, int indent) const {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("파일을 쓸 수 없습니다: " + path);
    }
    file << dump(indent);
}

} // namespace json
