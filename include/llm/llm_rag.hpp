#pragma once
#define NOMINMAX

// llm_rag.hpp -- Zero-dependency single-header C++ retrieval-augmented generation.
// Chunk text, embed via OpenAI, store in flat-file vector index, retrieve, generate.
//
// USAGE:
//   #define LLM_RAG_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_rag.hpp"
//
// Requires: libcurl

#include <cstdint>
#include <string>
#include <vector>

namespace llm {

struct RagConfig {
    size_t chunk_size    = 512;
    size_t chunk_overlap = 64;

    std::string embed_api_key;
    std::string embed_model = "text-embedding-3-small";

    std::string llm_api_key;
    std::string llm_model = "gpt-4o-mini";
    int         max_tokens = 1024;

    size_t      top_k      = 5;
    std::string index_path = ".rag_index";
};

struct Chunk {
    std::string          id;
    std::string          text;
    std::string          source;
    size_t               chunk_index = 0;
    std::vector<float>   embedding;
};

class RagPipeline {
public:
    explicit RagPipeline(RagConfig config);

    void ingest(const std::string& text, const std::string& source_name = "");
    void ingest_file(const std::string& filepath);

    struct RagResult {
        std::string          answer;
        std::vector<Chunk>   retrieved_chunks;
        std::string          augmented_prompt;
    };
    RagResult query(const std::string& question);

    std::vector<Chunk> retrieve(const std::string& question);

    void   save_index();
    void   load_index();
    size_t chunk_count() const;

private:
    RagConfig            m_cfg;
    std::vector<Chunk>   m_chunks;

    std::vector<float>   embed_text(const std::string& text);
    std::string          complete(const std::string& prompt);
    float                cosine_sim(const std::vector<float>& a,
                                    const std::vector<float>& b) const;
};

/// Chunk a text into overlapping passages.
std::vector<std::string> chunk_text(const std::string& text,
                                     size_t chunk_size,
                                     size_t overlap);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

#ifdef LLM_RAG_IMPLEMENTATION

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <curl/curl.h>

namespace llm {
namespace detail {

struct CurlH {
    CURL* h = nullptr;
    CurlH() : h(curl_easy_init()) {}
    ~CurlH() { if (h) curl_easy_cleanup(h); }
    CurlH(const CurlH&) = delete; CurlH& operator=(const CurlH&) = delete;
    bool ok() const { return h != nullptr; }
};
struct CurlSl {
    curl_slist* l = nullptr;
    ~CurlSl() { if (l) curl_slist_free_all(l); }
    CurlSl(const CurlSl&) = delete; CurlSl& operator=(const CurlSl&) = delete;
    CurlSl() = default;
    void append(const char* s) { l = curl_slist_append(l, s); }
};
static size_t wcb(char* p, size_t s, size_t n, void* ud) {
    static_cast<std::string*>(ud)->append(p, s*n); return s*n;
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
    curl_easy_perform(c.h);
    return resp;
}

static std::string jesc(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        switch(c){case '"':o+="\\\"";break;case '\\':o+="\\\\";break;
                  case '\n':o+="\\n";break;case '\r':o+="\\r";break;case '\t':o+="\\t";break;
                  default: if(c<0x20){char b[8];snprintf(b,sizeof(b),"\\u%04x",c);o+=b;}
                           else o+=static_cast<char>(c);}
    }
    return o;
}

static std::vector<float> parse_embedding(const std::string& j) {
    auto p = j.find("\"embedding\"");
    if (p == std::string::npos) return {};
    p = j.find('[', p);
    if (p == std::string::npos) return {};
    ++p;
    std::vector<float> out;
    while (p < j.size()) {
        while (p < j.size() && (j[p]==' '||j[p]=='\n'||j[p]==',')) ++p;
        if (p >= j.size() || j[p]==']') break;
        char* end = nullptr;
        float v = std::strtof(j.c_str()+p, &end);
        if (end == j.c_str()+p) break;
        out.push_back(v);
        p = static_cast<size_t>(end - j.c_str());
    }
    return out;
}

static std::string jstr(const std::string& j, const std::string& k) {
    std::string pat = "\""+k+"\"";
    auto p = j.find(pat);
    if (p==std::string::npos) return {};
    p += pat.size();
    while(p<j.size()&&(j[p]==':'||j[p]==' '))++p;
    if(p>=j.size()||j[p]!='"')return {};
    ++p;
    std::string v;
    while(p<j.size()&&j[p]!='"'){
        if(j[p]=='\\'&&p+1<j.size()){
            char e=j[++p];switch(e){case'n':v+='\n';break;case't':v+='\t';break;
                                    case'"':v+='"';break;default:v+=e;}
        } else v+=j[p];
        ++p;
    }
    return v;
}

static std::string msg_content(const std::string& j) {
    auto p = j.find("\"message\"");
    if (p==std::string::npos) p = j.rfind("\"content\"");
    if (p==std::string::npos) return {};
    return jstr(j.substr(p), "content");
}

// Generate simple ID
static std::string make_id(const std::string& source, size_t idx) {
    std::ostringstream ss;
    ss << source << "_" << idx;
    return ss.str();
}

} // namespace detail

// ---------------------------------------------------------------------------
// chunk_text
// ---------------------------------------------------------------------------

std::vector<std::string> chunk_text(const std::string& text,
                                     size_t chunk_size, size_t overlap) {
    std::vector<std::string> chunks;
    if (text.empty() || chunk_size == 0) return chunks;

    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = std::min(pos + chunk_size, text.size());

        // Try to break at sentence boundary (". " followed by non-lowercase)
        if (end < text.size()) {
            size_t search_start = (end > overlap) ? end - overlap : 0;
            size_t best = std::string::npos;
            for (size_t i = end; i > search_start; --i) {
                if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
                    if (i + 1 < text.size() && text[i+1] == ' ') {
                        best = i + 2;
                        break;
                    }
                }
            }
            if (best != std::string::npos && best > pos) end = best;
        }

        chunks.push_back(text.substr(pos, end - pos));
        if (end >= text.size()) break;
        pos = (end > overlap) ? end - overlap : end;
    }
    return chunks;
}

// ---------------------------------------------------------------------------
// RagPipeline
// ---------------------------------------------------------------------------

RagPipeline::RagPipeline(RagConfig config) : m_cfg(std::move(config)) {
    load_index();
}

std::vector<float> RagPipeline::embed_text(const std::string& text) {
    std::ostringstream ss;
    ss << "{\"model\":\"" << detail::jesc(m_cfg.embed_model) << "\","
       << "\"input\":\"" << detail::jesc(text) << "\"}";
    std::string resp = detail::http_post(
        "https://api.openai.com/v1/embeddings", ss.str(), m_cfg.embed_api_key);
    return detail::parse_embedding(resp);
}

std::string RagPipeline::complete(const std::string& prompt) {
    std::ostringstream ss;
    ss << "{\"model\":\"" << detail::jesc(m_cfg.llm_model) << "\","
       << "\"max_tokens\":" << m_cfg.max_tokens << ","
       << "\"messages\":[{\"role\":\"user\",\"content\":\""
       << detail::jesc(prompt) << "\"}]}";
    std::string resp = detail::http_post(
        "https://api.openai.com/v1/chat/completions", ss.str(), m_cfg.llm_api_key);
    return detail::msg_content(resp);
}

float RagPipeline::cosine_sim(const std::vector<float>& a,
                               const std::vector<float>& b) const {
    float dot=0,na=0,nb=0;
    size_t n = std::min(a.size(),b.size());
    for(size_t i=0;i<n;++i){dot+=a[i]*b[i];na+=a[i]*a[i];nb+=b[i]*b[i];}
    float d = std::sqrt(na)*std::sqrt(nb);
    return d>0?dot/d:0;
}

void RagPipeline::ingest(const std::string& text, const std::string& source_name) {
    auto passages = chunk_text(text, m_cfg.chunk_size, m_cfg.chunk_overlap);
    for (size_t i = 0; i < passages.size(); ++i) {
        Chunk c;
        c.id          = detail::make_id(source_name.empty() ? "doc" : source_name, m_chunks.size());
        c.text        = passages[i];
        c.source      = source_name;
        c.chunk_index = i;
        c.embedding   = embed_text(passages[i]);
        m_chunks.push_back(std::move(c));
    }
}

void RagPipeline::ingest_file(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f) return;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    ingest(content, filepath);
}

std::vector<Chunk> RagPipeline::retrieve(const std::string& question) {
    auto qemb = embed_text(question);
    std::vector<std::pair<float,size_t>> scored;
    scored.reserve(m_chunks.size());
    for (size_t i = 0; i < m_chunks.size(); ++i)
        scored.push_back({cosine_sim(qemb, m_chunks[i].embedding), i});
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b){ return a.first > b.first; });
    std::vector<Chunk> out;
    size_t k = std::min(m_cfg.top_k, scored.size());
    for (size_t i = 0; i < k; ++i)
        out.push_back(m_chunks[scored[i].second]);
    return out;
}

RagPipeline::RagResult RagPipeline::query(const std::string& question) {
    RagResult r;
    r.retrieved_chunks = retrieve(question);

    std::ostringstream ctx;
    ctx << "Context:\n";
    for (size_t i = 0; i < r.retrieved_chunks.size(); ++i)
        ctx << "[" << i+1 << "] " << r.retrieved_chunks[i].text << "\n\n";
    ctx << "Question: " << question << "\nAnswer:";
    r.augmented_prompt = ctx.str();
    r.answer = complete(r.augmented_prompt);
    return r;
}

// Binary index: [uint32 count] per chunk: [uint32 id_len][id][uint32 text_len][text]
// [uint32 source_len][source][uint32 chunk_index][uint32 dim][float*dim]
void RagPipeline::save_index() {
    std::ofstream f(m_cfg.index_path, std::ios::binary);
    if (!f) return;
    auto w32 = [&](uint32_t v){ f.write(reinterpret_cast<const char*>(&v),4); };
    auto wstr = [&](const std::string& s){
        w32(static_cast<uint32_t>(s.size()));
        f.write(s.data(), static_cast<std::streamsize>(s.size()));
    };
    w32(static_cast<uint32_t>(m_chunks.size()));
    for (const auto& c : m_chunks) {
        wstr(c.id); wstr(c.text); wstr(c.source);
        w32(static_cast<uint32_t>(c.chunk_index));
        w32(static_cast<uint32_t>(c.embedding.size()));
        f.write(reinterpret_cast<const char*>(c.embedding.data()),
                static_cast<std::streamsize>(c.embedding.size()*sizeof(float)));
    }
}

void RagPipeline::load_index() {
    std::ifstream f(m_cfg.index_path, std::ios::binary);
    if (!f) return;
    m_chunks.clear();
    auto r32 = [&]{ uint32_t v=0; f.read(reinterpret_cast<char*>(&v),4); return v; };
    auto rstr = [&]{ uint32_t n=r32(); std::string s(n,'\0'); f.read(s.data(),n); return s; };
    uint32_t count = r32();
    m_chunks.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        Chunk c;
        c.id    = rstr(); c.text = rstr(); c.source = rstr();
        c.chunk_index = r32();
        uint32_t dim = r32();
        c.embedding.resize(dim);
        f.read(reinterpret_cast<char*>(c.embedding.data()),
               static_cast<std::streamsize>(dim*sizeof(float)));
        m_chunks.push_back(std::move(c));
    }
}

size_t RagPipeline::chunk_count() const { return m_chunks.size(); }

} // namespace llm

#endif // LLM_RAG_IMPLEMENTATION
