#pragma once

// llm_format.hpp — Zero-dependency single-header C++ structured output enforcer.
//
// Usage:
//   #define LLM_FORMAT_IMPLEMENTATION in exactly one .cpp file before including.
//   All other translation units: just #include "llm_format.hpp".

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>

namespace llm {

// ---------------------------------------------------------------------------
// JsonValue
// ---------------------------------------------------------------------------

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type        type    = Type::Null;
    bool        bool_val = false;
    double      num_val  = 0.0;
    std::string str_val;
    std::vector<JsonValue>             arr_val;
    std::map<std::string, JsonValue>   obj_val;

    bool is_null()   const { return type == Type::Null;   }
    bool is_bool()   const { return type == Type::Bool;   }
    bool is_number() const { return type == Type::Number; }
    bool is_string() const { return type == Type::String; }
    bool is_array()  const { return type == Type::Array;  }
    bool is_object() const { return type == Type::Object; }

    std::string as_string() const { return str_val; }
    double      as_number() const { return num_val;  }
    bool        as_bool()   const { return bool_val; }

    const JsonValue& operator[](const std::string& key) const;
    const JsonValue& operator[](size_t idx) const;
    bool   contains(const std::string& key) const;
    size_t size() const;
};

// ---------------------------------------------------------------------------
// JSON parsing and serialization
// ---------------------------------------------------------------------------

JsonValue   parse_json(const std::string& json_str);
std::string to_json(const JsonValue& val, bool pretty = false);

// ---------------------------------------------------------------------------
// Schema types
// ---------------------------------------------------------------------------

struct Field {
    std::string name;
    std::string type;        // "string", "number", "bool", "array", "object"
    bool        required    = true;
    std::string description;
};

struct Schema {
    std::vector<Field> fields;
    std::string        name;
};

// ---------------------------------------------------------------------------
// Enforcement API
// ---------------------------------------------------------------------------

struct FormatConfig {
    int  max_retries    = 3;
    bool strip_markdown = true;
    bool allow_extra_fields = true;
};

struct FormatResult {
    JsonValue   value;
    int         attempts_used = 0;
    bool        valid         = false;
    std::string raw_response;
};

struct ValidationResult {
    bool                     valid = false;
    std::vector<std::string> errors;
};

ValidationResult validate(const JsonValue& value, const Schema& schema,
                          bool allow_extra_fields = true);
std::string      schema_to_prompt(const Schema& schema);

FormatResult enforce_schema(
    const std::string& base_prompt,
    const Schema& schema,
    std::function<std::string(const std::string& prompt)> llm_fn,
    const FormatConfig& config = {});

} // namespace llm

// ---------------------------------------------------------------------------
// IMPLEMENTATION
// ---------------------------------------------------------------------------
#ifdef LLM_FORMAT_IMPLEMENTATION

namespace llm {
namespace detail {

// ---- JSON parser state ----
struct Parser {
    const std::string& src;
    size_t pos = 0;

    char peek() const {
        return pos < src.size() ? src[pos] : '\0';
    }
    char advance() {
        return pos < src.size() ? src[pos++] : '\0';
    }
    void skip_ws() {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
            ++pos;
    }
    bool at_end() const { return pos >= src.size(); }
};

static JsonValue parse_value(Parser& p);

static std::string parse_string(Parser& p) {
    // Caller must have already consumed the opening '"'
    std::string result;
    while (!p.at_end()) {
        char c = p.advance();
        if (c == '"') return result;
        if (c == '\\') {
            if (p.at_end()) break;
            char esc = p.advance();
            switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'u': {
                    // \uXXXX — parse 4 hex digits
                    if (p.pos + 4 > p.src.size()) { result += '?'; break; }
                    std::string hex = p.src.substr(p.pos, 4);
                    p.pos += 4;
                    uint32_t codepoint = 0;
                    for (char h : hex) {
                        codepoint <<= 4;
                        if (h >= '0' && h <= '9') codepoint += static_cast<uint32_t>(h - '0');
                        else if (h >= 'a' && h <= 'f') codepoint += static_cast<uint32_t>(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') codepoint += static_cast<uint32_t>(h - 'A' + 10);
                    }
                    // Encode as UTF-8
                    if (codepoint < 0x80) {
                        result += static_cast<char>(codepoint);
                    } else if (codepoint < 0x800) {
                        result += static_cast<char>(0xC0 | (codepoint >> 6));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (codepoint >> 12));
                        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    break;
                }
                default: result += esc; break;
            }
        } else {
            result += c;
        }
    }
    return result; // Malformed — return what we have
}

static JsonValue parse_number(Parser& p, char first) {
    std::string s;
    s += first;
    while (!p.at_end()) {
        char c = p.peek();
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            c == '.' || c == 'e' || c == 'E' ||
            c == '+' || c == '-') {
            s += c;
            p.advance();
        } else {
            break;
        }
    }
    JsonValue v;
    v.type = JsonValue::Type::Number;
    try {
        v.num_val = std::stod(s);
    } catch (...) {
        v.num_val = 0.0;
    }
    return v;
}

static JsonValue parse_array(Parser& p) {
    JsonValue v;
    v.type = JsonValue::Type::Array;
    p.skip_ws();
    if (!p.at_end() && p.peek() == ']') { p.advance(); return v; }
    while (!p.at_end()) {
        p.skip_ws();
        v.arr_val.push_back(parse_value(p));
        p.skip_ws();
        if (p.at_end()) break;
        char c = p.advance();
        if (c == ']') break;
        // c should be ','
    }
    return v;
}

static JsonValue parse_object(Parser& p) {
    JsonValue v;
    v.type = JsonValue::Type::Object;
    p.skip_ws();
    if (!p.at_end() && p.peek() == '}') { p.advance(); return v; }
    while (!p.at_end()) {
        p.skip_ws();
        if (p.at_end()) break;
        // Expect '"'
        char q = p.advance();
        if (q != '"') break; // Malformed
        std::string key = parse_string(p);
        p.skip_ws();
        if (!p.at_end()) p.advance(); // ':'
        p.skip_ws();
        JsonValue val = parse_value(p);
        v.obj_val[key] = std::move(val);
        p.skip_ws();
        if (p.at_end()) break;
        char c = p.advance();
        if (c == '}') break;
        // c should be ','
    }
    return v;
}

static JsonValue parse_value(Parser& p) {
    p.skip_ws();
    if (p.at_end()) return JsonValue{};

    char c = p.advance();

    if (c == '"') {
        JsonValue v;
        v.type    = JsonValue::Type::String;
        v.str_val = parse_string(p);
        return v;
    }
    if (c == '{') return parse_object(p);
    if (c == '[') return parse_array(p);
    if (c == 't') {
        // true
        if (p.pos + 3 <= p.src.size()) p.pos += 3; // 'rue'
        JsonValue v; v.type = JsonValue::Type::Bool; v.bool_val = true; return v;
    }
    if (c == 'f') {
        // false
        if (p.pos + 4 <= p.src.size()) p.pos += 4; // 'alse'
        JsonValue v; v.type = JsonValue::Type::Bool; v.bool_val = false; return v;
    }
    if (c == 'n') {
        // null
        if (p.pos + 3 <= p.src.size()) p.pos += 3; // 'ull'
        return JsonValue{};
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        return parse_number(p, c);
    }
    return JsonValue{}; // Unknown
}

// ---- to_json helpers ----
static void serialize(const JsonValue& v, std::ostringstream& oss,
                      bool pretty, int indent) {
    auto ind = [&](int depth) {
        if (!pretty) return;
        oss << '\n';
        for (int i = 0; i < depth * 2; ++i) oss << ' ';
    };

    switch (v.type) {
        case JsonValue::Type::Null:   oss << "null"; break;
        case JsonValue::Type::Bool:   oss << (v.bool_val ? "true" : "false"); break;
        case JsonValue::Type::Number: {
            double d = v.num_val;
            if (d == std::floor(d) && std::isfinite(d) &&
                std::abs(d) < 1e15) {
                oss << static_cast<long long>(d);
            } else {
                oss << d;
            }
            break;
        }
        case JsonValue::Type::String: {
            oss << '"';
            for (char ch : v.str_val) {
                switch (ch) {
                    case '"':  oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n";  break;
                    case '\r': oss << "\\r";  break;
                    case '\t': oss << "\\t";  break;
                    default:   oss << ch;     break;
                }
            }
            oss << '"';
            break;
        }
        case JsonValue::Type::Array: {
            oss << '[';
            bool first = true;
            for (const auto& elem : v.arr_val) {
                if (!first) oss << ',';
                first = false;
                ind(indent + 1);
                serialize(elem, oss, pretty, indent + 1);
            }
            if (pretty && !v.arr_val.empty()) ind(indent);
            oss << ']';
            break;
        }
        case JsonValue::Type::Object: {
            oss << '{';
            bool first = true;
            for (const auto& kv : v.obj_val) {
                if (!first) oss << ',';
                first = false;
                ind(indent + 1);
                oss << '"' << kv.first << '"' << ':';
                if (pretty) oss << ' ';
                serialize(kv.second, oss, pretty, indent + 1);
            }
            if (pretty && !v.obj_val.empty()) ind(indent);
            oss << '}';
            break;
        }
    }
}

// ---- strip_markdown ----
static std::string strip_markdown_fences(const std::string& s) {
    // Find first ``` occurrence
    size_t open = s.find("```");
    if (open == std::string::npos) return s;

    // Advance past the opening fence line (skip optional "json" tag and newline)
    size_t content_start = open + 3;
    // Skip optional language tag on same line
    while (content_start < s.size() && s[content_start] != '\n' && s[content_start] != '\r')
        ++content_start;
    // Skip the newline(s)
    while (content_start < s.size() && (s[content_start] == '\n' || s[content_start] == '\r'))
        ++content_start;

    // Find last ``` — search backwards
    size_t close = s.rfind("```");
    if (close == std::string::npos || close <= open) return s.substr(content_start);

    // Walk back from close to find the beginning of the closing fence line
    size_t content_end = close;
    // Trim trailing newline before the closing fence
    while (content_end > content_start &&
           (s[content_end - 1] == '\n' || s[content_end - 1] == '\r'))
        --content_end;

    if (content_end <= content_start) return "";
    return s.substr(content_start, content_end - content_start);
}

} // namespace detail

// ---------------------------------------------------------------------------
// JsonValue members
// ---------------------------------------------------------------------------

static const JsonValue s_null_sentinel{};

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (type != Type::Object) return s_null_sentinel;
    auto it = obj_val.find(key);
    if (it == obj_val.end()) return s_null_sentinel;
    return it->second;
}

const JsonValue& JsonValue::operator[](size_t idx) const {
    if (type != Type::Array || idx >= arr_val.size()) return s_null_sentinel;
    return arr_val[idx];
}

bool JsonValue::contains(const std::string& key) const {
    if (type != Type::Object) return false;
    return obj_val.find(key) != obj_val.end();
}

size_t JsonValue::size() const {
    if (type == Type::Array)  return arr_val.size();
    if (type == Type::Object) return obj_val.size();
    return 0;
}

// ---------------------------------------------------------------------------
// parse_json / to_json
// ---------------------------------------------------------------------------

JsonValue parse_json(const std::string& json_str) {
    detail::Parser p{json_str, 0};
    return detail::parse_value(p);
}

std::string to_json(const JsonValue& val, bool pretty) {
    std::ostringstream oss;
    detail::serialize(val, oss, pretty, 0);
    return oss.str();
}

// ---------------------------------------------------------------------------
// validate
// ---------------------------------------------------------------------------

ValidationResult validate(const JsonValue& value, const Schema& schema,
                          bool allow_extra_fields) {
    ValidationResult result;
    result.valid = true;

    if (!value.is_object()) {
        result.valid = false;
        result.errors.push_back("Expected a JSON object at the top level");
        return result;
    }

    // Check required fields and types
    for (const auto& field : schema.fields) {
        if (!value.contains(field.name)) {
            if (field.required) {
                result.valid = false;
                result.errors.push_back("Missing required field: \"" + field.name + "\"");
            }
            continue;
        }

        const JsonValue& fv = value[field.name];

        bool type_ok = false;
        if      (field.type == "string") type_ok = fv.is_string();
        else if (field.type == "number") type_ok = fv.is_number();
        else if (field.type == "bool")   type_ok = fv.is_bool();
        else if (field.type == "array")  type_ok = fv.is_array();
        else if (field.type == "object") type_ok = fv.is_object();
        else                             type_ok = true; // Unknown type — accept

        if (!type_ok) {
            result.valid = false;
            result.errors.push_back("Field \"" + field.name + "\" has wrong type: expected " +
                                    field.type);
        }
    }

    // Check for extra fields if not allowed
    if (!allow_extra_fields) {
        for (const auto& kv : value.obj_val) {
            bool found = false;
            for (const auto& f : schema.fields) {
                if (f.name == kv.first) { found = true; break; }
            }
            if (!found) {
                result.valid = false;
                result.errors.push_back("Unexpected field: \"" + kv.first + "\"");
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// schema_to_prompt
// ---------------------------------------------------------------------------

std::string schema_to_prompt(const Schema& schema) {
    std::ostringstream oss;
    oss << "Respond with a valid JSON object matching this schema:\n{\n";
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        const auto& f = schema.fields[i];
        oss << "  \"" << f.name << "\": <" << f.type << ">";
        if (!f.description.empty()) oss << " — " << f.description;
        if (i + 1 < schema.fields.size()) oss << ",";
        oss << "\n";
    }
    oss << "}\n";
    oss << "Respond with only the JSON object, no explanation, no markdown fences.";
    return oss.str();
}

// ---------------------------------------------------------------------------
// enforce_schema
// ---------------------------------------------------------------------------

FormatResult enforce_schema(
    const std::string& base_prompt,
    const Schema& schema,
    std::function<std::string(const std::string& prompt)> llm_fn,
    const FormatConfig& config)
{
    FormatResult result;

    std::string prompt = base_prompt + "\n\n" + schema_to_prompt(schema);

    for (int attempt = 0; attempt < config.max_retries; ++attempt) {
        result.attempts_used = attempt + 1;

        std::string raw = llm_fn(prompt);
        result.raw_response = raw;

        std::string text = raw;
        if (config.strip_markdown) {
            text = detail::strip_markdown_fences(raw);
        }

        // Trim whitespace
        size_t start = text.find_first_not_of(" \t\r\n");
        size_t end   = text.find_last_not_of(" \t\r\n");
        if (start != std::string::npos) {
            text = text.substr(start, end - start + 1);
        }

        JsonValue parsed = parse_json(text);
        ValidationResult vr = validate(parsed, schema, config.allow_extra_fields);

        if (vr.valid) {
            result.value = std::move(parsed);
            result.valid = true;
            return result;
        }

        // Build correction prompt for next attempt
        if (attempt + 1 < config.max_retries) {
            std::string errors_joined;
            for (size_t i = 0; i < vr.errors.size(); ++i) {
                if (i > 0) errors_joined += "; ";
                errors_joined += vr.errors[i];
            }
            prompt = "Your previous response was invalid. Errors: " + errors_joined +
                     ". Please respond again with only valid JSON matching the schema.\n\n" +
                     schema_to_prompt(schema);
        }
    }

    return result;
}

} // namespace llm

#endif // LLM_FORMAT_IMPLEMENTATION
