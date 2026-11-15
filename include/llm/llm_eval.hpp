#pragma once
#define NOMINMAX

// llm_eval.hpp — Zero-dependency single-header C++ LLM evaluation library.
// Run prompts N times, measure variance, compare models, score responses.
//
// USAGE:
//   #define LLM_EVAL_IMPLEMENTATION  // in exactly ONE .cpp file
//   #include "llm_eval.hpp"
//
// Requires: libcurl

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <curl/curl.h>

namespace llm {

struct EvalConfig {
    std::string api_key;
    std::string model          = "gpt-4o-mini";
    int         runs           = 10;
    double      temperature    = 0.7;
    std::string system_prompt;
    bool        verbose        = false;
};

struct EvalRun {
    int         run_index   = 0;
    std::string response;
    double      latency_ms  = 0.0;
    size_t      token_count = 0;
    bool        success     = true;
    std::string error;
};

struct EvalMetrics {
    double avg_length     = 0.0;
    double length_stddev  = 0.0;
    double length_cv      = 0.0;   ///< coefficient of variation = stddev/mean
    double avg_similarity = 0.0;   ///< pairwise Jaccard on word sets
    double min_similarity = 1.0;
    double max_similarity = 0.0;
    double avg_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    double success_rate   = 0.0;
    size_t total_runs     = 0;
    bool   high_variance  = false; ///< CV > 0.3
    bool   inconsistent   = false; ///< avg_similarity < 0.5
    bool   unreliable     = false; ///< success_rate < 0.9
};

struct EvalResult {
    std::string           prompt;
    std::string           model;
    std::vector<EvalRun>  runs;
    EvalMetrics           metrics;
    std::string           summary; ///< one-paragraph human-readable summary
};

struct ComparisonResult {
    EvalResult  model_a;
    EvalResult  model_b;
    std::string winner;             ///< "a", "b", or "tie"
    std::string comparison_summary;
};

struct ScoreResult {
    double                   score = 0.0; ///< 0.0–1.0
    std::vector<std::string> found;
    std::vector<std::string> missing;
};

/// Run `config.runs` completions of the same prompt and compute consistency metrics.
EvalResult evaluate(const std::string& prompt, const EvalConfig& config);

/// Compare two model configs on the same prompt.
ComparisonResult compare(
    const std::string& prompt,
    const EvalConfig&  config_a,
    const EvalConfig&  config_b
);

/// Compare two system prompts on the same model and user prompt.
ComparisonResult compare_prompts(
    const std::string& user_prompt,
    const std::string& system_a,
    const std::string& system_b,
    const EvalConfig&  base_config
);

/// Score a response against expected keywords. Returns 0.0–1.0.
ScoreResult score_response(const std::string& response,
                            const std::vector<std::string>& expected_keywords);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_EVAL_IMPLEMENTATION

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <numeric>

namespace llm {
namespace detail {

struct EvalBuf { std::string data; };
static size_t eval_write_cb(char* p, size_t s, size_t n, void* ud) {
    static_cast<EvalBuf*>(ud)->data.append(p, s * n); return s * n;
}

static std::string eval_escape(const std::string& s) {
    std::string o; o.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break; case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break; case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default: if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
                     else o += static_cast<char>(c);
        }
    }
    return o;
}

static std::string eval_str_val(const std::string& j, const std::string& k) {
    std::string pat = "\"" + k + "\"";
    auto p = j.find(pat); if (p == std::string::npos) return {};
    p += pat.size();
    while (p < j.size() && (j[p] == ' ' || j[p] == '\t' || j[p] == ':')) ++p;
    if (p >= j.size() || j[p] != '"') return {};
    ++p; std::string r;
    while (p < j.size() && j[p] != '"') {
        if (j[p] == '\\' && p + 1 < j.size()) {
            char e = j[p + 1];
            switch (e) { case '"': r += '"'; break; case '\\': r += '\\'; break;
                         case 'n': r += '\n'; break; case 't': r += '\t'; break;
                         default: r += e; } p += 2;
        } else r += j[p++];
    }
    return r;
}

static std::string single_call(const EvalConfig& cfg, const std::string& prompt) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string body = "{\"model\":\"" + eval_escape(cfg.model) + "\","
                       "\"temperature\":" + std::to_string(cfg.temperature) + ","
                       "\"messages\":[";
    if (!cfg.system_prompt.empty())
        body += "{\"role\":\"system\",\"content\":\"" + eval_escape(cfg.system_prompt) + "\"},";
    body += "{\"role\":\"user\",\"content\":\"" + eval_escape(prompt) + "\"}]}";

    EvalBuf buf;
    curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, ("Authorization: Bearer " + cfg.api_key).c_str());
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    hdrs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, eval_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       60L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    long code = 0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) throw std::runtime_error(curl_easy_strerror(res));
    if (code >= 400) throw std::runtime_error("HTTP " + std::to_string(code));
    return buf.data;
}

static double jaccard(const std::string& a, const std::string& b) {
    auto tok = [](const std::string& s) {
        std::set<std::string> w;
        std::istringstream ss(s); std::string t;
        while (ss >> t) w.insert(t);
        return w;
    };
    auto wa = tok(a), wb = tok(b);
    size_t inter = 0;
    for (const auto& w : wa) if (wb.count(w)) ++inter;
    size_t uni = wa.size() + wb.size() - inter;
    return uni == 0 ? 1.0 : static_cast<double>(inter) / uni;
}

static EvalMetrics compute_metrics(const std::vector<EvalRun>& runs) {
    EvalMetrics m; m.total_runs = runs.size();
    if (runs.empty()) return m;

    size_t ok = 0; double sl = 0, st = 0;
    std::vector<double> lens, lats;
    for (const auto& r : runs) {
        if (r.success) ++ok;
        double l = static_cast<double>(r.response.size());
        sl += l; st += r.latency_ms; lens.push_back(l); lats.push_back(r.latency_ms);
    }
    m.success_rate = static_cast<double>(ok) / runs.size();
    m.avg_length   = sl / runs.size();
    m.avg_latency_ms = st / runs.size();

    double var = 0;
    for (double l : lens) { double d = l - m.avg_length; var += d * d; }
    m.length_stddev = std::sqrt(var / runs.size());
    m.length_cv = m.avg_length > 0 ? m.length_stddev / m.avg_length : 0.0;

    std::sort(lats.begin(), lats.end());
    size_t p95i = static_cast<size_t>(0.95 * lats.size());
    if (p95i >= lats.size()) p95i = lats.size() - 1;
    m.p95_latency_ms = lats[p95i];

    size_t pairs = 0; double ss = 0; m.min_similarity = 1.0; m.max_similarity = 0.0;
    for (size_t i = 0; i < runs.size(); ++i)
        for (size_t j = i + 1; j < runs.size(); ++j) {
            double s = jaccard(runs[i].response, runs[j].response);
            ss += s; m.min_similarity = std::min(m.min_similarity, s);
            m.max_similarity = std::max(m.max_similarity, s); ++pairs;
        }
    m.avg_similarity = pairs > 0 ? ss / pairs : 1.0;

    m.high_variance = m.length_cv > 0.3;
    m.inconsistent  = m.avg_similarity < 0.5;
    m.unreliable    = m.success_rate < 0.9;
    return m;
}

static std::string build_summary(const EvalResult& r) {
    std::ostringstream s;
    s << r.model << " over " << r.metrics.total_runs << " runs: "
      << "success=" << static_cast<int>(r.metrics.success_rate * 100) << "%, "
      << "avg_len=" << static_cast<int>(r.metrics.avg_length) << "ch, "
      << "CV=" << static_cast<int>(r.metrics.length_cv * 100) << "%, "
      << "avg_sim=" << static_cast<int>(r.metrics.avg_similarity * 100) << "%, "
      << "p95_lat=" << static_cast<int>(r.metrics.p95_latency_ms) << "ms.";
    if (r.metrics.high_variance) s << " [HIGH_VARIANCE]";
    if (r.metrics.inconsistent)  s << " [INCONSISTENT]";
    if (r.metrics.unreliable)    s << " [UNRELIABLE]";
    return s.str();
}

} // namespace detail

// ---------------------------------------------------------------------------

EvalResult evaluate(const std::string& prompt, const EvalConfig& config) {
    EvalResult result; result.prompt = prompt; result.model = config.model;
    for (int i = 0; i < config.runs; ++i) {
        if (config.verbose) fprintf(stderr, "[eval] run %d/%d\n", i + 1, config.runs);
        EvalRun run; run.run_index = i;
        auto t0 = std::chrono::steady_clock::now();
        try {
            std::string resp = detail::single_call(config, prompt);
            run.response     = detail::eval_str_val(resp, "content");
            run.success      = true;
        } catch (const std::exception& e) { run.success = false; run.error = e.what(); }
        run.latency_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();
        result.runs.push_back(run);
    }
    result.metrics = detail::compute_metrics(result.runs);
    result.summary = detail::build_summary(result);
    return result;
}

ComparisonResult compare(const std::string& prompt,
                          const EvalConfig& ca, const EvalConfig& cb) {
    ComparisonResult cr;
    cr.model_a = evaluate(prompt, ca);
    cr.model_b = evaluate(prompt, cb);
    double sa = cr.model_a.metrics.avg_similarity - cr.model_a.metrics.length_cv + cr.model_a.metrics.success_rate;
    double sb = cr.model_b.metrics.avg_similarity - cr.model_b.metrics.length_cv + cr.model_b.metrics.success_rate;
    cr.winner = sa > sb ? "a" : sb > sa ? "b" : "tie";
    cr.comparison_summary = "A: " + cr.model_a.summary + " | B: " + cr.model_b.summary + " | Winner: " + cr.winner;
    return cr;
}

ComparisonResult compare_prompts(const std::string& user_prompt,
                                  const std::string& system_a, const std::string& system_b,
                                  const EvalConfig& base) {
    EvalConfig ca = base; ca.system_prompt = system_a;
    EvalConfig cb = base; cb.system_prompt = system_b;
    return compare(user_prompt, ca, cb);
}

ScoreResult score_response(const std::string& response,
                            const std::vector<std::string>& kws) {
    ScoreResult sr;
    if (kws.empty()) { sr.score = 1.0; return sr; }
    std::string lower = response;
    for (char& c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    for (const auto& kw : kws) {
        std::string lkw = kw;
        for (char& c : lkw) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (lower.find(lkw) != std::string::npos) sr.found.push_back(kw);
        else sr.missing.push_back(kw);
    }
    sr.score = static_cast<double>(sr.found.size()) / kws.size();
    return sr;
}

} // namespace llm
#endif // LLM_EVAL_IMPLEMENTATION
