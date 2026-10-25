#include "json.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>

#include "exceptions.hpp"

namespace {

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    Json parseDocument() {
        Json value = parseValue();
        skipWhitespace();
        if (pos_ != text_.size()) fail("unexpected trailing characters");
        return value;
    }

private:
    [[noreturn]] void fail(const std::string& what) const {
        throw BadRequestException("Invalid JSON at position " + std::to_string(pos_) + ": " + what);
    }

    void skipWhitespace() {
        while (pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\n' ||
                                       text_[pos_] == '\r' || text_[pos_] == '\t')) {
            ++pos_;
        }
    }

    char peek() {
        skipWhitespace();
        if (pos_ >= text_.size()) fail("unexpected end of input");
        return text_[pos_];
    }

    void expect(char c) {
        if (peek() != c) fail(std::string("expected '") + c + "'");
        ++pos_;
    }

    bool consumeLiteral(const char* literal) {
        std::size_t len = std::char_traits<char>::length(literal);
        if (text_.compare(pos_, len, literal) == 0) {
            pos_ += len;
            return true;
        }
        return false;
    }

    Json parseValue() {
        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return Json(parseString());
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
        if (consumeLiteral("true")) return Json(true);
        if (consumeLiteral("false")) return Json(false);
        if (consumeLiteral("null")) return Json();
        fail("unexpected character");
    }

    Json parseObject() {
        expect('{');
        Json obj = Json::object();
        if (peek() == '}') {
            ++pos_;
            return obj;
        }
        while (true) {
            if (peek() != '"') fail("expected string key");
            std::string key = parseString();
            expect(':');
            obj[key] = parseValue();
            char c = peek();
            ++pos_;
            if (c == '}') break;
            if (c != ',') fail("expected ',' or '}'");
        }
        return obj;
    }

    Json parseArray() {
        expect('[');
        Json arr = Json::array();
        if (peek() == ']') {
            ++pos_;
            return arr;
        }
        while (true) {
            arr.push(parseValue());
            char c = peek();
            ++pos_;
            if (c == ']') break;
            if (c != ',') fail("expected ',' or ']'");
        }
        return arr;
    }

    static void appendUtf8(std::string& out, unsigned code) {
        if (code < 0x80) {
            out += static_cast<char>(code);
        } else if (code < 0x800) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (code >> 18));
            out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    unsigned parseHex4() {
        if (pos_ + 4 > text_.size()) fail("truncated unicode escape");
        unsigned code = 0;
        for (int i = 0; i < 4; ++i) {
            char h = text_[pos_++];
            code <<= 4;
            if (h >= '0' && h <= '9') code |= static_cast<unsigned>(h - '0');
            else if (h >= 'a' && h <= 'f') code |= static_cast<unsigned>(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') code |= static_cast<unsigned>(h - 'A' + 10);
            else fail("bad unicode escape");
        }
        return code;
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (true) {
            if (pos_ >= text_.size()) fail("unterminated string");
            char c = text_[pos_++];
            if (c == '"') break;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos_ >= text_.size()) fail("unterminated escape");
            char e = text_[pos_++];
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned code = parseHex4();
                    if (code >= 0xD800 && code <= 0xDBFF && text_.compare(pos_, 2, "\\u") == 0) {
                        pos_ += 2;
                        unsigned low = parseHex4();
                        code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    }
                    appendUtf8(out, code);
                    break;
                }
                default: fail("bad escape character");
            }
        }
        return out;
    }

    Json parseNumber() {
        std::size_t start = pos_;
        if (text_[pos_] == '-') ++pos_;
        while (pos_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[pos_])) ||
                                       text_[pos_] == '.' || text_[pos_] == 'e' || text_[pos_] == 'E' ||
                                       text_[pos_] == '+' || text_[pos_] == '-')) {
            ++pos_;
        }
        try {
            return Json(std::stod(text_.substr(start, pos_ - start)));
        } catch (const std::exception&) {
            fail("bad number");
        }
    }

    const std::string& text_;
    std::size_t pos_ = 0;
};

void escapeString(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

const char* kindName(Json::Kind kind) {
    switch (kind) {
        case Json::Kind::Null: return "null";
        case Json::Kind::Bool: return "boolean";
        case Json::Kind::Number: return "number";
        case Json::Kind::String: return "string";
        case Json::Kind::Array: return "array";
        case Json::Kind::Object: return "object";
    }
    return "value";
}

}  // namespace

Json Json::array() {
    Json j;
    j.kind_ = Kind::Array;
    return j;
}

Json Json::object() {
    Json j;
    j.kind_ = Kind::Object;
    return j;
}

Json Json::parse(const std::string& text) { return Parser(text).parseDocument(); }

bool Json::asBool() const {
    if (!isBool()) throw BadRequestException(std::string("Expected boolean, got ") + kindName(kind_));
    return bool_;
}

double Json::asNumber() const {
    if (!isNumber()) throw BadRequestException(std::string("Expected number, got ") + kindName(kind_));
    return number_;
}

long long Json::asInt() const { return static_cast<long long>(std::llround(asNumber())); }

const std::string& Json::asString() const {
    if (!isString()) throw BadRequestException(std::string("Expected string, got ") + kindName(kind_));
    return string_;
}

const Json::Array& Json::asArray() const {
    if (!isArray()) throw BadRequestException(std::string("Expected array, got ") + kindName(kind_));
    return array_;
}

const Json::Object& Json::asObject() const {
    if (!isObject()) throw BadRequestException(std::string("Expected object, got ") + kindName(kind_));
    return object_;
}

Json& Json::operator[](const std::string& key) {
    if (isNull()) kind_ = Kind::Object;
    if (!isObject()) throw BadRequestException("Cannot index a non-object JSON value");
    for (auto& member : object_) {
        if (member.first == key) return member.second;
    }
    object_.emplace_back(key, Json());
    return object_.back().second;
}

const Json& Json::get(const std::string& key) const {
    static const Json null;
    if (!isObject()) return null;
    for (const auto& member : object_) {
        if (member.first == key) return member.second;
    }
    return null;
}

bool Json::has(const std::string& key) const { return !get(key).isNull(); }

std::string Json::getString(const std::string& key, const std::string& fallback) const {
    const Json& v = get(key);
    if (v.isNull()) return fallback;
    if (v.isNumber()) return std::to_string(v.asInt());
    if (!v.isString()) throw BadRequestException("Field \"" + key + "\" must be a string");
    return v.string_;
}

long long Json::getInt(const std::string& key, long long fallback) const {
    const Json& v = get(key);
    if (v.isNull()) return fallback;
    if (v.isString()) {
        if (v.string_.empty()) return fallback;
        try {
            std::size_t used = 0;
            long long n = std::stoll(v.string_, &used);
            if (used != v.string_.size()) throw std::invalid_argument("trailing");
            return n;
        } catch (const std::exception&) {
            throw BadRequestException("Field \"" + key + "\" must be a whole number");
        }
    }
    if (!v.isNumber()) throw BadRequestException("Field \"" + key + "\" must be a number");
    return v.asInt();
}

bool Json::getBool(const std::string& key, bool fallback) const {
    const Json& v = get(key);
    if (v.isNull()) return fallback;
    if (!v.isBool()) throw BadRequestException("Field \"" + key + "\" must be true or false");
    return v.bool_;
}

void Json::push(Json value) {
    if (isNull()) kind_ = Kind::Array;
    if (!isArray()) throw BadRequestException("Cannot push onto a non-array JSON value");
    array_.push_back(std::move(value));
}

std::size_t Json::size() const {
    if (isArray()) return array_.size();
    if (isObject()) return object_.size();
    return 0;
}

std::string Json::dump(int indent) const {
    std::string out;
    dumpTo(out, indent, 0);
    return out;
}

void Json::dumpTo(std::string& out, int indent, int depth) const {
    auto newline = [&](int d) {
        if (indent < 0) return;
        out += '\n';
        out.append(static_cast<std::size_t>(indent * d), ' ');
    };

    switch (kind_) {
        case Kind::Null: out += "null"; break;
        case Kind::Bool: out += bool_ ? "true" : "false"; break;
        case Kind::Number: {
            if (std::isfinite(number_) && number_ == std::floor(number_) && std::fabs(number_) < 1e15) {
                out += std::to_string(static_cast<long long>(number_));
            } else {
                std::ostringstream ss;
                ss << number_;
                out += ss.str();
            }
            break;
        }
        case Kind::String: escapeString(out, string_); break;
        case Kind::Array: {
            out += '[';
            for (std::size_t i = 0; i < array_.size(); ++i) {
                if (i) out += ',';
                newline(depth + 1);
                array_[i].dumpTo(out, indent, depth + 1);
            }
            if (!array_.empty()) newline(depth);
            out += ']';
            break;
        }
        case Kind::Object: {
            out += '{';
            for (std::size_t i = 0; i < object_.size(); ++i) {
                if (i) out += ',';
                newline(depth + 1);
                escapeString(out, object_[i].first);
                out += indent < 0 ? ":" : ": ";
                object_[i].second.dumpTo(out, indent, depth + 1);
            }
            if (!object_.empty()) newline(depth);
            out += '}';
            break;
        }
    }
}
