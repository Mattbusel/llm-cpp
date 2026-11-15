#pragma once
#define NOMINMAX

// llm_embed.hpp -- Zero-dependency single-header C++ text embeddings,
// cosine similarity, and nearest-neighbor vector search via OpenAI API.
//
// USAGE:
//   #define LLM_EMBED_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_embed.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace llm {

using Embedding = std::vector<float>;

struct EmbedConfig {
    std::string api_key;
    std::string model      = "text-embedding-3-small";
    int         dimensions = 1536;
};

// ---------------------------------------------------------------------------
// Embedding API
// ---------------------------------------------------------------------------

/// Fetch embedding for a single string.
Embedding embed(const std::string& text, const EmbedConfig& config);

/// Fetch embeddings for multiple strings in one API call.
std::vector<Embedding> embed_batch(
    const std::vector<std::string>& texts,
    const EmbedConfig& config
);

// ---------------------------------------------------------------------------
// Similarity functions
// ---------------------------------------------------------------------------

float cosine_similarity(const Embedding& a, const Embedding& b);
float dot_product(const Embedding& a, const Embedding& b);
float euclidean_distance(const Embedding& a, const Embedding& b);

// ---------------------------------------------------------------------------
// Flat-file vector store
// ---------------------------------------------------------------------------

struct VectorEntry {
    std::string id;
    std::string text;
    Embedding   embedding;
    std::map<std::string, std::string> metadata;
};

class VectorStore {
public:
    explicit VectorStore(const std::string& filepath);

    void add(const std::string& id, const std::string& text,
             const Embedding& embedding,
             std::map<std::string, std::string> metadata = {});

    void remove(const std::string& id);

    struct SearchResult {
        VectorEntry entry;
        float       similarity = 0.0f;
    };

    /// Return top-k most similar entries by cosine similarity.
    std::vector<SearchResult> search(const Embedding& query, size_t top_k = 5) const;

    void save();
    void load();

    size_t size() const;

private:
    std::string              m_filepath;
    std::vector<VectorEntry> m_entries;
};

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

#ifdef LLM_EMBED_IMPLEMENTATION

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <curl/curl.h>

namespace llm {
namespace detail {

// ---------------------------------------------------------------------------
// libcurl RAII
// ---------------------------------------------------------------------------
struct CurlHandle {
    CURL* h = nullptr;
    CurlHandle() : h(curl_easy_init()) {}
    ~CurlHandle() { if (h) curl_easy_cleanup(h); }
    CurlHandle(const CurlHandle&) = delete;
    CurlHandle& operator=(const CurlHandle&) = delete;
    bool ok() const { return h != nullptr; }
};

struct CurlSlist {
    curl_slist* list = nullptr;
    ~CurlSlist() { if (list) curl_slist_free_all(list); }
    CurlSlist(const CurlSlist&) = delete;
    CurlSlist& operator=(const CurlSlist&) = delete;
    CurlSlist() = default;
    void append(const char* s) { list = curl_slist_append(list, s); }
};

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* ud) {
    static_cast<std::string*>(ud)->append(ptr, size * nmemb);
    return size * nmemb;
}

static std::string http_post(const std::string& url, const std::string& body,
                               const std::string& api_key) {
    CurlHandle curl;
    if (!curl.ok()) return {};
    CurlSlist headers;
    headers.append("Content-Type: application/json");
    headers.append(("Authorization: Bearer " + api_key).c_str());

    std::string response;
    curl_easy_setopt(curl.h, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl.h, CURLOPT_HTTPHEADER,     headers.list);
    curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(curl.h, CURLOPT_WRITEFUNCTION,  write_cb);
    curl_easy_setopt(curl.h, CURLOPT_WRITEDATA,      &response);
    curl_easy_setopt(curl.h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(curl.h);
    return response;
}

// ---------------------------------------------------------------------------
// Minimal JSON helpers
// ---------------------------------------------------------------------------
static std::string json_escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
                else out += static_cast<char>(c);
        }
    }
    return out;
}

// Parse a JSON array of floats after the first occurrence of "embedding":
static Embedding parse_embedding(const std::string& json) {
    auto pos = json.find("\"embedding\"");
    if (pos == std::string::npos) return {};
    pos = json.find('[', pos);
    if (pos == std::string::npos) return {};
    ++pos;
    Embedding out;
    while (pos < json.size()) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == ',')) ++pos;
        if (pos >= json.size() || json[pos] == ']') break;
        char* end = nullptr;
        float val = std::strtof(json.c_str() + pos, &end);
        if (end == json.c_str() + pos) break;
        out.push_back(val);
        pos = static_cast<size_t>(end - json.c_str());
    }
    return out;
}

// Parse array of embeddings from batch response (data[i].embedding)
static std::vector<Embedding> parse_batch_embeddings(const std::string& json, size_t count) {
    std::vector<Embedding> result;
    size_t pos = 0;
    while (result.size() < count) {
        auto found = json.find("\"embedding\"", pos);
        if (found == std::string::npos) break;
        auto arr_start = json.find('[', found);
        if (arr_start == std::string::npos) break;
        Embedding emb;
        size_t i = arr_start + 1;
        while (i < json.size()) {
            while (i < json.size() && (json[i] == ' ' || json[i] == '\n' || json[i] == ',')) ++i;
            if (i >= json.size() || json[i] == ']') break;
            char* end = nullptr;
            float val = std::strtof(json.c_str() + i, &end);
            if (end == json.c_str() + i) break;
            emb.push_back(val);
            i = static_cast<size_t>(end - json.c_str());
        }
        result.push_back(std::move(emb));
        pos = (i < json.size()) ? i + 1 : json.size();
    }
    return result;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Public: embed
// ---------------------------------------------------------------------------

Embedding embed(const std::string& text, const EmbedConfig& cfg) {
    std::ostringstream ss;
    ss << "{\"model\":\"" << detail::json_escape(cfg.model) << "\","
       << "\"input\":\"" << detail::json_escape(text) << "\"";
    if (cfg.dimensions > 0) ss << ",\"dimensions\":" << cfg.dimensions;
    ss << "}";

    std::string resp = detail::http_post(
        "https://api.openai.com/v1/embeddings", ss.str(), cfg.api_key);
    return detail::parse_embedding(resp);
}

std::vector<Embedding> embed_batch(const std::vector<std::string>& texts,
                                    const EmbedConfig& cfg) {
    if (texts.empty()) return {};
    std::ostringstream ss;
    ss << "{\"model\":\"" << detail::json_escape(cfg.model) << "\","
       << "\"input\":[";
    for (size_t i = 0; i < texts.size(); ++i) {
        if (i) ss << ',';
        ss << '"' << detail::json_escape(texts[i]) << '"';
    }
    ss << "]";
    if (cfg.dimensions > 0) ss << ",\"dimensions\":" << cfg.dimensions;
    ss << "}";

    std::string resp = detail::http_post(
        "https://api.openai.com/v1/embeddings", ss.str(), cfg.api_key);
    return detail::parse_batch_embeddings(resp, texts.size());
}

// ---------------------------------------------------------------------------
// Similarity
// ---------------------------------------------------------------------------

float cosine_similarity(const Embedding& a, const Embedding& b) {
    if (a.size() != b.size())
        throw std::invalid_argument("cosine_similarity: dimension mismatch ("
            + std::to_string(a.size()) + " vs " + std::to_string(b.size()) + ")");
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) { dot += a[i]*b[i]; na += a[i]*a[i]; nb += b[i]*b[i]; }
    float denom = std::sqrt(na) * std::sqrt(nb);
    return denom > 0.0f ? dot / denom : 0.0f;
}

float dot_product(const Embedding& a, const Embedding& b) {
    if (a.size() != b.size())
        throw std::invalid_argument("dot_product: dimension mismatch ("
            + std::to_string(a.size()) + " vs " + std::to_string(b.size()) + ")");
    float s = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) s += a[i] * b[i];
    return s;
}

float euclidean_distance(const Embedding& a, const Embedding& b) {
    if (a.size() != b.size())
        throw std::invalid_argument("euclidean_distance: dimension mismatch ("
            + std::to_string(a.size()) + " vs " + std::to_string(b.size()) + ")");
    float s = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) { float d = a[i]-b[i]; s += d*d; }
    return std::sqrt(s);
}

// ---------------------------------------------------------------------------
// VectorStore
// ---------------------------------------------------------------------------

VectorStore::VectorStore(const std::string& filepath) : m_filepath(filepath) {
    load();
}

void VectorStore::add(const std::string& id, const std::string& text,
                      const Embedding& embedding,
                      std::map<std::string, std::string> metadata) {
    remove(id); // replace if exists
    VectorEntry e;
    e.id = id; e.text = text; e.embedding = embedding; e.metadata = std::move(metadata);
    m_entries.push_back(std::move(e));
}

void VectorStore::remove(const std::string& id) {
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
                       [&](const VectorEntry& e){ return e.id == id; }),
        m_entries.end());
}

std::vector<VectorStore::SearchResult> VectorStore::search(
    const Embedding& query, size_t top_k) const {

    std::vector<SearchResult> results;
    results.reserve(m_entries.size());
    for (const auto& e : m_entries) {
        SearchResult r;
        r.entry = e;
        r.similarity = cosine_similarity(query, e.embedding);
        results.push_back(r);
    }
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b){
                  return a.similarity > b.similarity;
              });
    if (results.size() > top_k) results.resize(top_k);
    return results;
}

// Binary format: [uint32 count] per entry: [uint32 id_len][id][uint32 text_len][text]
// [uint32 dim][float * dim][uint32 meta_count] per meta: [uint32 klen][k][uint32 vlen][v]
void VectorStore::save() {
    std::ofstream f(m_filepath, std::ios::binary);
    if (!f) return;
    auto w32 = [&](uint32_t v){ f.write(reinterpret_cast<const char*>(&v), 4); };
    auto wstr = [&](const std::string& s){ w32(static_cast<uint32_t>(s.size())); f.write(s.data(), static_cast<std::streamsize>(s.size())); };
    w32(static_cast<uint32_t>(m_entries.size()));
    for (const auto& e : m_entries) {
        wstr(e.id);
        wstr(e.text);
        w32(static_cast<uint32_t>(e.embedding.size()));
        f.write(reinterpret_cast<const char*>(e.embedding.data()),
                static_cast<std::streamsize>(e.embedding.size() * sizeof(float)));
        w32(static_cast<uint32_t>(e.metadata.size()));
        for (const auto& [k, v] : e.metadata) { wstr(k); wstr(v); }
    }
}

void VectorStore::load() {
    std::ifstream f(m_filepath, std::ios::binary);
    if (!f) return;
    m_entries.clear();
    auto r32 = [&]{ uint32_t v = 0; f.read(reinterpret_cast<char*>(&v), 4); return v; };
    auto rstr = [&]{ uint32_t n = r32(); std::string s(n, '\0'); f.read(s.data(), n); return s; };
    uint32_t count = r32();
    m_entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        VectorEntry e;
        e.id   = rstr();
        e.text = rstr();
        uint32_t dim = r32();
        e.embedding.resize(dim);
        f.read(reinterpret_cast<char*>(e.embedding.data()),
               static_cast<std::streamsize>(dim * sizeof(float)));
        uint32_t mc = r32();
        for (uint32_t j = 0; j < mc; ++j) {
            std::string k = rstr(), v = rstr();
            e.metadata[k] = v;
        }
        m_entries.push_back(std::move(e));
    }
}

size_t VectorStore::size() const { return m_entries.size(); }

} // namespace llm

#endif // LLM_EMBED_IMPLEMENTATION
