#pragma once

#include <string>
#include <utility>
#include <vector>

// Minimal JSON value: enough for the REST API and the save file.
// Objects keep insertion order so saved files stay readable.
class Json {
public:
    enum class Kind { Null, Bool, Number, String, Array, Object };

    using Array = std::vector<Json>;
    using Member = std::pair<std::string, Json>;
    using Object = std::vector<Member>;

    Json() = default;
    Json(std::nullptr_t) {}
    Json(bool value) : kind_(Kind::Bool), bool_(value) {}
    Json(int value) : kind_(Kind::Number), number_(value) {}
    Json(long value) : kind_(Kind::Number), number_(static_cast<double>(value)) {}
    Json(long long value) : kind_(Kind::Number), number_(static_cast<double>(value)) {}
    Json(double value) : kind_(Kind::Number), number_(value) {}
    Json(const char* value) : kind_(Kind::String), string_(value) {}
    Json(std::string value) : kind_(Kind::String), string_(std::move(value)) {}

    static Json array();
    static Json object();
    static Json parse(const std::string& text);

    Kind kind() const { return kind_; }
    bool isNull() const { return kind_ == Kind::Null; }
    bool isBool() const { return kind_ == Kind::Bool; }
    bool isNumber() const { return kind_ == Kind::Number; }
    bool isString() const { return kind_ == Kind::String; }
    bool isArray() const { return kind_ == Kind::Array; }
    bool isObject() const { return kind_ == Kind::Object; }

    bool asBool() const;
    double asNumber() const;
    long long asInt() const;
    const std::string& asString() const;
    const Array& asArray() const;
    const Object& asObject() const;

    // Object access. The non-const overload inserts a null member if missing.
    Json& operator[](const std::string& key);
    const Json& get(const std::string& key) const;
    bool has(const std::string& key) const;

    // Typed lookups used when reading request bodies; wrong types throw
    // BadRequestException naming the offending field.
    std::string getString(const std::string& key, const std::string& fallback = "") const;
    long long getInt(const std::string& key, long long fallback = 0) const;
    bool getBool(const std::string& key, bool fallback = false) const;

    void push(Json value);
    std::size_t size() const;

    std::string dump(int indent = -1) const;

private:
    void dumpTo(std::string& out, int indent, int depth) const;

    Kind kind_ = Kind::Null;
    bool bool_ = false;
    double number_ = 0;
    std::string string_;
    Array array_;
    Object object_;
};
