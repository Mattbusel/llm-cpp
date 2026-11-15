#pragma once
#define NOMINMAX

// llm_vision.hpp -- Zero-dependency single-header C++ multimodal image+text.
// Base64-encode images inline, supports OpenAI GPT-4V and Anthropic Claude vision.
//
// USAGE:
//   #define LLM_VISION_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_vision.hpp"
//
// Requires: libcurl

#include <functional>
#include <string>
#include <vector>

namespace llm {

enum class VisionProvider { OpenAI, Anthropic };

struct VisionImage {
    std::string data;      // raw binary bytes (loaded from file or provided)
    std::string mime_type; // e.g. "image/jpeg", "image/png"
    std::string url;       // alternative: pass URL instead of raw bytes
};

struct VisionConfig {
    std::string    api_key;
    std::string    model      = "gpt-4o";
    VisionProvider provider   = VisionProvider::OpenAI;
    int            max_tokens = 1024;
    double         temperature = 0.0;
};

using StreamCallback = std::function<void(const std::string& delta)>;

/// Load image bytes from a file path.
VisionImage load_image(const std::string& filepath,
                        const std::string& mime_type = "image/jpeg");

/// Send image(s) + text prompt; return model response.
std::string vision(const std::string& prompt,
                   const std::vector<VisionImage>& images,
                   const VisionConfig& config);

/// Streaming variant — calls on_delta for each token, returns full reply.
std::string vision_stream(const std::string& prompt,
                           const std::vector<VisionImage>& images,
                           const VisionConfig& config,
                           StreamCallback on_delta);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_VISION_IMPLEMENTATION

#include <cstdio>
#include <cstring>
#include <fstream>
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
                              const std::string& key,
                              const std::string& extra_header = "") {
    CurlH c; if (!c.ok()) return {};
    CurlSl h;
    h.append("Content-Type: application/json");
    h.append(("Authorization: Bearer " + key).c_str());
    if (!extra_header.empty()) h.append(extra_header.c_str());
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

struct StreamCtx { std::string buf; StreamCallback cb; std::string full; };

static size_t stream_wcb(char* p, size_t s, size_t n, void* ud) {
    auto* ctx = static_cast<StreamCtx*>(ud);
    ctx->buf.append(p, s * n);
    size_t pos = 0;
    while (true) {
        auto nl = ctx->buf.find('\n', pos);
        if (nl == std::string::npos) break;
        std::string line = ctx->buf.substr(pos, nl - pos);
        pos = nl + 1;
        if (line.size() > 6 && line.substr(0, 6) == "data: ") {
            std::string payload = line.substr(6);
            if (payload == "[DONE]") break;
            auto dc = payload.find("\"content\":\"");
            if (dc == std::string::npos) {
                // Anthropic streaming: "delta":{"type":"text_delta","text":"..."}
                dc = payload.find("\"text\":\"");
                if (dc != std::string::npos) dc += 8;
                else dc = std::string::npos;
            } else {
                dc += 11;
            }
            if (dc != std::string::npos) {
                std::string delta;
                while (dc < payload.size() && payload[dc] != '"') {
                    if (payload[dc] == '\\' && dc + 1 < payload.size()) {
                        ++dc;
                        char e = payload[dc];
                        if (e == 'n') delta += '\n'; else if (e == 't') delta += '\t'; else delta += e;
                    } else delta += payload[dc];
                    ++dc;
                }
                if (!delta.empty()) { ctx->full += delta; if (ctx->cb) ctx->cb(delta); }
            }
        }
    }
    ctx->buf = ctx->buf.substr(pos);
    return s * n;
}

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const std::string& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    const auto* src = reinterpret_cast<const unsigned char*>(data.data());
    size_t n = data.size();
    while (i + 2 < n) {
        unsigned int v = (static_cast<unsigned>(src[i]) << 16) |
                         (static_cast<unsigned>(src[i+1]) << 8) |
                          static_cast<unsigned>(src[i+2]);
        out += B64[(v >> 18) & 0x3F];
        out += B64[(v >> 12) & 0x3F];
        out += B64[(v >>  6) & 0x3F];
        out += B64[(v      ) & 0x3F];
        i += 3;
    }
    if (i < n) {
        unsigned int v = static_cast<unsigned>(src[i]) << 16;
        if (i + 1 < n) v |= static_cast<unsigned>(src[i+1]) << 8;
        out += B64[(v >> 18) & 0x3F];
        out += B64[(v >> 12) & 0x3F];
        out += (i + 1 < n) ? B64[(v >> 6) & 0x3F] : '=';
        out += '=';
    }
    return out;
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

static std::string extract_content(const std::string& resp) {
    // OpenAI: choices[0].message.content
    auto p = resp.find("\"message\"");
    if (p == std::string::npos) p = resp.rfind("\"content\"");
    if (p == std::string::npos) return {};
    p = resp.find("\"content\"", p);
    if (p == std::string::npos) return {};
    p += 9;
    while (p < resp.size() && (resp[p] == ':' || resp[p] == ' ')) ++p;
    if (p >= resp.size() || resp[p] != '"') return {};
    ++p;
    std::string v;
    while (p < resp.size() && resp[p] != '"') {
        if (resp[p] == '\\' && p + 1 < resp.size()) {
            char e = resp[++p];
            if (e == 'n') v += '\n'; else if (e == 't') v += '\t'; else v += e;
        } else v += resp[p];
        ++p;
    }
    return v;
}

static std::string build_openai_body(const std::string& prompt,
                                      const std::vector<VisionImage>& images,
                                      const VisionConfig& cfg, bool stream) {
    // content array: text + image_url parts
    std::string content = "[";
    content += "{\"type\":\"text\",\"text\":\"" + jesc(prompt) + "\"}";
    for (const auto& img : images) {
        content += ",{\"type\":\"image_url\",\"image_url\":{\"url\":\"";
        if (!img.url.empty()) {
            content += jesc(img.url);
        } else {
            content += "data:" + jesc(img.mime_type) + ";base64," + base64_encode(img.data);
        }
        content += "\"}}";
    }
    content += "]";

    std::string body = "{\"model\":\"" + jesc(cfg.model) + "\","
                       "\"max_tokens\":" + std::to_string(cfg.max_tokens) + ","
                       "\"messages\":[{\"role\":\"user\",\"content\":" + content + "}]";
    if (stream) body += ",\"stream\":true";
    body += "}";
    return body;
}

static std::string build_anthropic_body(const std::string& prompt,
                                         const std::vector<VisionImage>& images,
                                         const VisionConfig& cfg, bool stream) {
    std::string content = "[";
    for (const auto& img : images) {
        content += "{\"type\":\"image\",\"source\":{";
        if (!img.url.empty()) {
            content += "\"type\":\"url\",\"url\":\"" + jesc(img.url) + "\"";
        } else {
            content += "\"type\":\"base64\","
                       "\"media_type\":\"" + jesc(img.mime_type) + "\","
                       "\"data\":\"" + base64_encode(img.data) + "\"";
        }
        content += "}},";
    }
    content += "{\"type\":\"text\",\"text\":\"" + jesc(prompt) + "\"}]";

    std::string body = "{\"model\":\"" + jesc(cfg.model) + "\","
                       "\"max_tokens\":" + std::to_string(cfg.max_tokens) + ","
                       "\"messages\":[{\"role\":\"user\",\"content\":" + content + "}]";
    if (stream) body += ",\"stream\":true";
    body += "}";
    return body;
}

} // namespace detail

// ---------------------------------------------------------------------------

VisionImage load_image(const std::string& filepath, const std::string& mime_type) {
    std::ifstream f(filepath, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open image: " + filepath);
    VisionImage img;
    img.data      = std::string((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
    img.mime_type = mime_type;
    return img;
}

std::string vision(const std::string& prompt,
                   const std::vector<VisionImage>& images,
                   const VisionConfig& cfg) {
    std::string body, resp;
    if (cfg.provider == VisionProvider::OpenAI) {
        body = detail::build_openai_body(prompt, images, cfg, false);
        resp = detail::http_post("https://api.openai.com/v1/chat/completions",
                                  body, cfg.api_key);
    } else {
        body = detail::build_anthropic_body(prompt, images, cfg, false);
        resp = detail::http_post("https://api.anthropic.com/v1/messages",
                                  body, cfg.api_key,
                                  "anthropic-version: 2023-06-01");
    }
    return detail::extract_content(resp);
}

std::string vision_stream(const std::string& prompt,
                           const std::vector<VisionImage>& images,
                           const VisionConfig& cfg,
                           StreamCallback on_delta) {
    std::string body;
    std::string url;
    std::string extra;
    if (cfg.provider == VisionProvider::OpenAI) {
        body = detail::build_openai_body(prompt, images, cfg, true);
        url  = "https://api.openai.com/v1/chat/completions";
    } else {
        body  = detail::build_anthropic_body(prompt, images, cfg, true);
        url   = "https://api.anthropic.com/v1/messages";
        extra = "anthropic-version: 2023-06-01";
    }

    detail::CurlH c; if (!c.ok()) return {};
    detail::CurlSl h;
    h.append("Content-Type: application/json");
    h.append(("Authorization: Bearer " + cfg.api_key).c_str());
    if (!extra.empty()) h.append(extra.c_str());
    detail::StreamCtx ctx;
    ctx.cb = on_delta;
    curl_easy_setopt(c.h, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(c.h, CURLOPT_HTTPHEADER,     h.l);
    curl_easy_setopt(c.h, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(c.h, CURLOPT_WRITEFUNCTION,  detail::stream_wcb);
    curl_easy_setopt(c.h, CURLOPT_WRITEDATA,      &ctx);
    curl_easy_setopt(c.h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,        120L);
    curl_easy_perform(c.h);
    return ctx.full;
}

} // namespace llm
#endif // LLM_VISION_IMPLEMENTATION
