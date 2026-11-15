#pragma once
#define NOMINMAX

// llm_chat.hpp -- Zero-dependency single-header C++ conversation manager.
// Multi-turn history, token-budget truncation, system prompt pinning,
// serialize/deserialize, chat() and chat_stream() functions.
//
// USAGE:
//   #define LLM_CHAT_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_chat.hpp"
//
// Requires: libcurl

#include <functional>
#include <string>
#include <vector>

namespace llm {

struct ChatMessage {
    std::string role;    // "system", "user", "assistant"
    std::string content;
};

struct ChatConfig {
    std::string api_key;
    std::string model         = "gpt-4o-mini";
    std::string system_prompt;
    int         max_tokens    = 1024;
    double      temperature   = 0.7;
    size_t      token_budget  = 4096; // estimated chars/4; truncates oldest non-system
    bool        verbose       = false;
};

using StreamCallback = std::function<void(const std::string& delta)>;

class Conversation {
public:
    explicit Conversation(ChatConfig config);

    // Send a user message and return the assistant reply.
    std::string chat(const std::string& user_message);

    // Send a user message with streaming delta callback; returns full reply.
    std::string chat_stream(const std::string& user_message,
                            StreamCallback on_delta);

    // Access history (includes system message if set).
    const std::vector<ChatMessage>& history() const;

    // Clear all messages except the pinned system prompt.
    void reset();

    // Serialize/deserialize history to/from JSONL string.
    std::string serialize() const;
    void        deserialize(const std::string& jsonl);

    size_t message_count() const;

private:
    ChatConfig               m_cfg;
    std::vector<ChatMessage> m_history;

    void   trim_to_budget();
    size_t estimate_tokens() const;

    std::string do_complete(bool stream, StreamCallback cb);
    std::string build_body(bool stream) const;
};

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_CHAT_IMPLEMENTATION

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <stdexcept>

#include <curl/curl.h>

namespace llm {
namespace detail {

struct CurlH {
    CURL* h = nullptr;
    CurlH() : h(curl_easy_init()) {}
    ~CurlH() { if (h) curl_easy_cleanup(h); }
    CurlH(const CurlH&) = delete;
    CurlH& operator=(const CurlH&) = delete;
    bool ok() const { return h != nullptr; }
};

struct CurlSl {
    curl_slist* l = nullptr;
    ~CurlSl() { if (l) curl_slist_free_all(l); }
    CurlSl(const CurlSl&) = delete;
    CurlSl& operator=(const CurlSl&) = delete;
    CurlSl() = default;
    void append(const char* s) { l = curl_slist_append(l, s); }
};

static size_t wcb(char* p, size_t s, size_t n, void* ud) {
    static_cast<std::string*>(ud)->append(p, s * n);
    return s * n;
}

static std::string http_post(const std::string& url, const std::string& body,
                              const std::string& key) {
    CurlH c; if (!c.ok()) return {};
    CurlSl h;
    h.append("Content-Type: application/json");
    h.append(("Authorization: Bearer " + key).c_str());
    std::string resp;
    curl_easy_setopt(c.h, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(c.h, CURLOPT_HTTPHEADER,     h.l);
    curl_easy_setopt(c.h, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(c.h, CURLOPT_WRITEFUNCTION,  wcb);
    curl_easy_setopt(c.h, CURLOPT_WRITEDATA,      &resp);
    curl_easy_setopt(c.h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,        120L);
    curl_easy_perform(c.h);
    return resp;
}

struct StreamCtx {
    std::string     buffer;
    StreamCallback  cb;
    std::string     full;
};

static size_t stream_wcb(char* p, size_t s, size_t n, void* ud) {
    auto* ctx = static_cast<StreamCtx*>(ud);
    ctx->buffer.append(p, s * n);
    // parse SSE lines
    size_t pos = 0;
    while (true) {
        auto nl = ctx->buffer.find('\n', pos);
        if (nl == std::string::npos) break;
        std::string line = ctx->buffer.substr(pos, nl - pos);
        pos = nl + 1;
        if (line.size() > 6 && line.substr(0, 6) == "data: ") {
            std::string payload = line.substr(6);
            if (payload == "[DONE]") break;
            // extract delta content
            auto dc = payload.find("\"content\":\"");
            if (dc != std::string::npos) {
                dc += 11;
                std::string delta;
                while (dc < payload.size() && payload[dc] != '"') {
                    if (payload[dc] == '\\' && dc + 1 < payload.size()) {
                        ++dc;
                        char e = payload[dc];
                        if (e == 'n') delta += '\n';
                        else if (e == 't') delta += '\t';
                        else delta += e;
                    } else {
                        delta += payload[dc];
                    }
                    ++dc;
                }
                if (!delta.empty()) {
                    ctx->full += delta;
                    if (ctx->cb) ctx->cb(delta);
                }
            }
        }
    }
    ctx->buffer = ctx->buffer.substr(pos);
    return s * n;
}

static std::string http_post_stream(const std::string& url, const std::string& body,
                                     const std::string& key, StreamCallback cb,
                                     std::string& full_out) {
    CurlH c; if (!c.ok()) return {};
    CurlSl h;
    h.append("Content-Type: application/json");
    h.append(("Authorization: Bearer " + key).c_str());
    StreamCtx ctx;
    ctx.cb = cb;
    curl_easy_setopt(c.h, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(c.h, CURLOPT_HTTPHEADER,     h.l);
    curl_easy_setopt(c.h, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(c.h, CURLOPT_WRITEFUNCTION,  stream_wcb);
    curl_easy_setopt(c.h, CURLOPT_WRITEDATA,      &ctx);
    curl_easy_setopt(c.h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,        120L);
    curl_easy_perform(c.h);
    full_out = ctx.full;
    return ctx.full;
}

static std::string jesc(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
                else o += static_cast<char>(c);
        }
    }
    return o;
}

static std::string jstr(const std::string& j, const std::string& k) {
    std::string pat = "\"" + k + "\"";
    auto p = j.find(pat);
    if (p == std::string::npos) return {};
    p += pat.size();
    while (p < j.size() && (j[p] == ':' || j[p] == ' ')) ++p;
    if (p >= j.size() || j[p] != '"') return {};
    ++p;
    std::string v;
    while (p < j.size() && j[p] != '"') {
        if (j[p] == '\\' && p + 1 < j.size()) {
            char e = j[++p];
            if (e == 'n') v += '\n';
            else if (e == 't') v += '\t';
            else v += e;
        } else {
            v += j[p];
        }
        ++p;
    }
    return v;
}

} // namespace detail

// ---------------------------------------------------------------------------

Conversation::Conversation(ChatConfig config) : m_cfg(std::move(config)) {
    if (!m_cfg.system_prompt.empty())
        m_history.push_back({"system", m_cfg.system_prompt});
}

std::string Conversation::build_body(bool stream) const {
    std::string msgs = "[";
    for (size_t i = 0; i < m_history.size(); ++i) {
        if (i) msgs += ",";
        msgs += "{\"role\":\"" + m_history[i].role + "\","
                "\"content\":\"" + detail::jesc(m_history[i].content) + "\"}";
    }
    msgs += "]";
    std::string body = "{\"model\":\"" + detail::jesc(m_cfg.model) + "\","
                       "\"max_tokens\":" + std::to_string(m_cfg.max_tokens) + ","
                       "\"temperature\":" + std::to_string(m_cfg.temperature) + ","
                       "\"messages\":" + msgs;
    if (stream) body += ",\"stream\":true";
    body += "}";
    return body;
}

size_t Conversation::estimate_tokens() const {
    size_t chars = 0;
    for (const auto& m : m_history) chars += m.content.size();
    return (chars + 3) / 4;
}

void Conversation::trim_to_budget() {
    // Keep system message (index 0 if role=="system"), trim oldest user/assistant pairs
    size_t sys = (!m_history.empty() && m_history[0].role == "system") ? 1 : 0;
    while (estimate_tokens() > m_cfg.token_budget && m_history.size() > sys + 1) {
        m_history.erase(m_history.begin() + static_cast<long>(sys));
    }
}

std::string Conversation::do_complete(bool stream, StreamCallback cb) {
    std::string body = build_body(stream);
    std::string reply;
    if (stream) {
        detail::http_post_stream(
            "https://api.openai.com/v1/chat/completions",
            body, m_cfg.api_key, cb, reply);
    } else {
        std::string resp = detail::http_post(
            "https://api.openai.com/v1/chat/completions",
            body, m_cfg.api_key);
        // parse content
        auto p = resp.find("\"message\"");
        if (p == std::string::npos) p = resp.rfind("\"content\"");
        if (p != std::string::npos)
            reply = detail::jstr(resp.substr(p), "content");
    }
    return reply;
}

std::string Conversation::chat(const std::string& user_message) {
    m_history.push_back({"user", user_message});
    trim_to_budget();
    std::string reply = do_complete(false, nullptr);
    m_history.push_back({"assistant", reply});
    return reply;
}

std::string Conversation::chat_stream(const std::string& user_message,
                                       StreamCallback on_delta) {
    m_history.push_back({"user", user_message});
    trim_to_budget();
    std::string reply = do_complete(true, on_delta);
    m_history.push_back({"assistant", reply});
    return reply;
}

const std::vector<ChatMessage>& Conversation::history() const {
    return m_history;
}

void Conversation::reset() {
    m_history.clear();
    if (!m_cfg.system_prompt.empty())
        m_history.push_back({"system", m_cfg.system_prompt});
}

std::string Conversation::serialize() const {
    std::string out;
    for (const auto& m : m_history) {
        out += "{\"role\":\"" + detail::jesc(m.role) + "\","
               "\"content\":\"" + detail::jesc(m.content) + "\"}\n";
    }
    return out;
}

void Conversation::deserialize(const std::string& jsonl) {
    m_history.clear();
    std::istringstream ss(jsonl);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        ChatMessage msg;
        msg.role    = detail::jstr(line, "role");
        msg.content = detail::jstr(line, "content");
        if (!msg.role.empty()) m_history.push_back(msg);
    }
}

size_t Conversation::message_count() const {
    return m_history.size();
}

} // namespace llm
#endif // LLM_CHAT_IMPLEMENTATION
