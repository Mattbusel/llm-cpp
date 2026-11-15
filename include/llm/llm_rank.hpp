#pragma once
#define NOMINMAX

// llm_rank.hpp -- Reranking: score and sort passages by relevance.
// BM25 local reranking (no API) + LLM reranking + hybrid mode.
// libcurl required only for LLM-based reranking.
//
// USAGE: #define LLM_RANK_IMPLEMENTATION in ONE .cpp file.

#include <string>
#include <vector>
#include <functional>
#include <cstddef>

namespace llm {

struct RankResult {
    std::string passage;
    double      score         = 0.0;
    int         original_rank = 0;
    int         new_rank      = 0;
};

// ---------- Local (BM25) ----------
struct LocalRankConfig {
    double bm25_weight    = 0.6;
    double keyword_weight = 0.4;
    double k1             = 1.5;
    double b              = 0.75;
};

std::vector<RankResult> rerank_local(
    const std::string& query,
    const std::vector<std::string>& passages,
    const LocalRankConfig& cfg = {});

// ---------- LLM-based ----------
struct LLMRankConfig {
    std::string api_key;
    std::string model       = "gpt-4o-mini";
    int         batch_size  = 5;
    std::string score_prompt =
        "On a scale of 0-10, how relevant is the following passage to the query? "
        "Respond with only a number.\nQuery: {query}\nPassage: {passage}";
};

std::vector<RankResult> rerank_llm(
    const std::string& query,
    const std::vector<std::string>& passages,
    const LLMRankConfig& cfg);

// ---------- Hybrid ----------
std::vector<RankResult> rerank_hybrid(
    const std::string& query,
    const std::vector<std::string>& passages,
    const LocalRankConfig& local_cfg,
    const LLMRankConfig& llm_cfg,
    size_t llm_top_k = 10);

// ---------- BM25 standalone ----------
class BM25 {
public:
    explicit BM25(const std::vector<std::string>& corpus,
                  double k1 = 1.5, double b = 0.75);
    std::vector<double> scores(const std::string& query) const;

private:
    struct Doc { std::vector<std::string> terms; size_t len; };
    std::vector<Doc>    docs_;
    std::vector<std::string> vocab_;
    std::vector<double> idf_;
    double avg_dl_ = 0.0;
    double k1_, b_;
    std::vector<std::string> tokenize(const std::string& s) const;
};

} // namespace llm

// ---------------------------------------------------------------------------
#ifdef LLM_RANK_IMPLEMENTATION

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <sstream>
#include <unordered_map>

#include <curl/curl.h>

namespace llm {
namespace detail_rank {

static std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> toks;
    std::string w;
    for (char c : s) {
        if (std::isalnum((unsigned char)c)) w += (char)std::tolower((unsigned char)c);
        else if (!w.empty()) { toks.push_back(w); w.clear(); }
    }
    if (!w.empty()) toks.push_back(w);
    return toks;
}

static double keyword_overlap(const std::string& query, const std::string& passage) {
    auto qt = tokenize(query);
    auto pt = tokenize(passage);
    if (qt.empty()) return 0.0;
    std::unordered_map<std::string,int> pm;
    for (auto& w : pt) pm[w]++;
    int hits = 0;
    for (auto& w : qt) if (pm.count(w)) ++hits;
    return (double)hits / qt.size();
}

static size_t wcb(char* p, size_t s, size_t n, void* ud) {
    static_cast<std::string*>(ud)->append(p, s * n); return s * n;
}

static std::string jesc(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else o += (char)c;
    }
    return o;
}

static std::string llm_call(const std::string& prompt, const LLMRankConfig& cfg) {
    CURL* curl = curl_easy_init();
    if (!curl) return "5";
    curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    hdrs = curl_slist_append(hdrs, ("Authorization: Bearer " + cfg.api_key).c_str());
    std::string body = "{\"model\":\"" + jesc(cfg.model) + "\","
        "\"max_tokens\":5,\"temperature\":0,"
        "\"messages\":[{\"role\":\"user\",\"content\":\"" + jesc(prompt) + "\"}]}";
    std::string resp;
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wcb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    // extract content
    auto p = resp.find("\"content\":\"");
    if (p == std::string::npos) return "5";
    p += 11;
    std::string val;
    while (p < resp.size() && resp[p] != '"') { val += resp[p++]; }
    return val;
}

static double parse_score(const std::string& s) {
    // scan from end to find last number (avoids grabbing preamble digits)
    size_t last = std::string::npos;
    for (size_t i = 0; i < s.size(); ++i)
        if (std::isdigit((unsigned char)s[i])) last = i;
    if (last == std::string::npos) return 0.5;
    double v = std::stod(s.substr(last));
    return std::max(0.0, std::min(10.0, v)) / 10.0;
}

static void assign_ranks(std::vector<RankResult>& results) {
    std::sort(results.begin(), results.end(),
              [](const RankResult& a, const RankResult& b){ return a.score > b.score; });
    for (int i = 0; i < (int)results.size(); ++i) results[i].new_rank = i + 1;
}

} // namespace detail_rank

// BM25 implementation
BM25::BM25(const std::vector<std::string>& corpus, double k1, double b)
    : k1_(k1), b_(b) {
    std::unordered_map<std::string, int> df;
    for (auto& doc : corpus) {
        auto toks = tokenize(doc);
        docs_.push_back({toks, toks.size()});
        avg_dl_ += toks.size();
        std::unordered_map<std::string,bool> seen;
        for (auto& t : toks) { if (!seen[t]) { df[t]++; seen[t]=true; } }
    }
    avg_dl_ = corpus.empty() ? 1.0 : avg_dl_ / corpus.size();

    // collect vocab
    for (auto& kv : df) vocab_.push_back(kv.first);
    double N = (double)corpus.size();
    for (auto& term : vocab_) {
        double n = df[term];
        idf_.push_back(std::log((N - n + 0.5) / (n + 0.5) + 1.0));
    }
}

std::vector<std::string> BM25::tokenize(const std::string& s) const {
    return detail_rank::tokenize(s);
}

std::vector<double> BM25::scores(const std::string& query) const {
    auto qtoks = tokenize(query);
    std::vector<double> sc(docs_.size(), 0.0);
    for (size_t qi = 0; qi < vocab_.size(); ++qi) {
        auto& term = vocab_[qi];
        bool in_query = false;
        for (auto& t : qtoks) if (t == term) { in_query = true; break; }
        if (!in_query) continue;
        for (size_t di = 0; di < docs_.size(); ++di) {
            int tf = 0;
            for (auto& t : docs_[di].terms) if (t == term) ++tf;
            double dl = docs_[di].len;
            double num = tf * (k1_ + 1.0);
            double den = tf + k1_ * (1.0 - b_ + b_ * dl / avg_dl_);
            sc[di] += idf_[qi] * num / den;
        }
    }
    return sc;
}

std::vector<RankResult> rerank_local(const std::string& query,
                                     const std::vector<std::string>& passages,
                                     const LocalRankConfig& cfg) {
    BM25 bm25(passages, cfg.k1, cfg.b);
    auto bm25_scores = bm25.scores(query);

    // normalize BM25 scores to 0-1
    double max_bm25 = *std::max_element(bm25_scores.begin(), bm25_scores.end());
    if (max_bm25 == 0.0) max_bm25 = 1.0;

    std::vector<RankResult> results;
    for (size_t i = 0; i < passages.size(); ++i) {
        RankResult r;
        r.passage      = passages[i];
        r.original_rank = (int)i + 1;
        double bm25_n  = bm25_scores[i] / max_bm25;
        double kw      = detail_rank::keyword_overlap(query, passages[i]);
        r.score        = cfg.bm25_weight * bm25_n + cfg.keyword_weight * kw;
        results.push_back(r);
    }
    detail_rank::assign_ranks(results);
    return results;
}

std::vector<RankResult> rerank_llm(const std::string& query,
                                   const std::vector<std::string>& passages,
                                   const LLMRankConfig& cfg) {
    std::vector<RankResult> results;
    for (size_t i = 0; i < passages.size(); ++i) {
        RankResult r;
        r.passage       = passages[i];
        r.original_rank = (int)i + 1;

        std::string prompt = cfg.score_prompt;
        // replace {query} and {passage}
        auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
            size_t p = 0;
            while ((p = s.find(from, p)) != std::string::npos) { s.replace(p, from.size(), to); p += to.size(); }
        };
        replace_all(prompt, "{query}", query);
        replace_all(prompt, "{passage}", passages[i].substr(0, 500));

        auto resp = detail_rank::llm_call(prompt, cfg);
        r.score = detail_rank::parse_score(resp);
        results.push_back(r);
    }
    detail_rank::assign_ranks(results);
    return results;
}

std::vector<RankResult> rerank_hybrid(const std::string& query,
                                      const std::vector<std::string>& passages,
                                      const LocalRankConfig& local_cfg,
                                      const LLMRankConfig& llm_cfg,
                                      size_t llm_top_k) {
    // Step 1: local rerank all
    auto local = rerank_local(query, passages, local_cfg);

    // Step 2: LLM rerank top-k
    std::vector<std::string> top_passages;
    for (size_t i = 0; i < std::min(llm_top_k, local.size()); ++i)
        top_passages.push_back(local[i].passage);

    auto llm_results = rerank_llm(query, top_passages, llm_cfg);

    // Merge: LLM results get LLM score, rest keep local score (scaled down)
    std::vector<RankResult> final_results;
    for (auto& r : llm_results) final_results.push_back(r);
    for (size_t i = llm_top_k; i < local.size(); ++i) {
        auto r = local[i];
        r.score *= 0.5; // discount non-LLM-reranked
        final_results.push_back(r);
    }
    detail_rank::assign_ranks(final_results);
    return final_results;
}

} // namespace llm
#endif // LLM_RANK_IMPLEMENTATION
