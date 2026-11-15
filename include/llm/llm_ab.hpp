#pragma once
#define NOMINMAX

// llm_ab.hpp -- A/B testing framework for LLM prompts and models.
// Welch t-test, Cohen d, custom scorers. libcurl required.
//
// USAGE: #define LLM_AB_IMPLEMENTATION in ONE .cpp file.

#include <string>
#include <vector>
#include <functional>
#include <cstddef>

namespace llm {

struct Variant {
    std::string name;
    std::string prompt;           // supports {input} placeholder
    std::string model;
    std::string api_key;
    std::string api_url = "https://api.openai.com/v1/chat/completions";
    double      temperature = 0.7;
};

struct ABSample {
    std::string input;
    std::string expected;         // optional ground truth
};

struct VariantResult {
    std::string name;
    std::vector<std::string> outputs;
    std::vector<double>      scores;
    double mean_score = 0.0;
    double std_dev    = 0.0;
    size_t n          = 0;
    std::string error;            // non-empty if a network/API error occurred
};

struct ABResult {
    VariantResult control;
    VariantResult treatment;
    double t_statistic = 0.0;
    double p_value     = 1.0;
    double cohens_d    = 0.0;
    bool   significant = false;
    double alpha       = 0.05;
    std::string winner;
    std::string summary;
};

struct ABConfig {
    std::string api_key;
    double alpha        = 0.05;
    long   timeout_secs = 30;
    std::function<double(const std::string&, const std::string&, const std::string&)> scorer;
};

VariantResult run_variant(
    const Variant& variant,
    const std::vector<ABSample>& samples,
    const ABConfig& cfg = {});

ABResult run_ab_test(
    const Variant& control,
    const Variant& treatment,
    const std::vector<ABSample>& samples,
    const ABConfig& cfg = {});

} // namespace llm

// ---------------------------------------------------------------------------
#ifdef LLM_AB_IMPLEMENTATION

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

#include <curl/curl.h>

namespace llm {
namespace detail_ab {

static size_t wcb(char* p, size_t s, size_t n, void* ud) {
    static_cast<std::string*>(ud)->append(p, s * n); return s * n;
}

static std::string jesc(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        if      (c == '"')  { o += '\\'; o += '"'; }
        else if (c == '\\') { o += '\\'; o += '\\'; }
        else if (c == '\n') { o += '\\'; o += 'n'; }
        else if (c == '\r') { o += '\\'; o += 'r'; }
        else if (c == '\t') { o += '\\'; o += 't'; }
        else o += static_cast<char>(c);
    }
    return o;
}

static std::string llm_call(const std::string& prompt, const Variant& v,
                             long timeout_secs, std::string& err) {
    CURL* c = curl_easy_init();
    if (!c) { err = "curl_easy_init failed"; return {}; }

    curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    hdrs = curl_slist_append(hdrs, ("Authorization: Bearer " + v.api_key).c_str());

    char tbuf[32]; snprintf(tbuf, sizeof(tbuf), "%.2f", v.temperature);
    std::string body = "{\"model\":\"" + jesc(v.model) + "\","
        "\"temperature\":" + tbuf + ","
        "\"messages\":[{\"role\":\"user\",\"content\":\"" + jesc(prompt) + "\"}]}";
    std::string resp;

    curl_easy_setopt(c, CURLOPT_URL, v.api_url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, wcb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeout_secs);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode rc = curl_easy_perform(c);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);

    if (rc != CURLE_OK) {
        err = std::string("curl error: ") + curl_easy_strerror(rc);
        return {};
    }
    if (resp.find("\"error\"") != std::string::npos) {
        auto ep = resp.find("\"message\":\"");
        if (ep != std::string::npos) {
            ep += 11;
            std::string emsg;
            while (ep < resp.size() && resp[ep] != '"') emsg += resp[ep++];
            err = "API error: " + emsg;
        } else {
            err = "API error (unknown)";
        }
        return {};
    }
    auto p2 = resp.find("\"content\":\"");
    if (p2 == std::string::npos) { err = "no content in response"; return {}; }
    p2 += 11;
    std::string out;
    while (p2 < resp.size() && resp[p2] != '"') {
        char ch = resp[p2];
        if (ch == '\\' && p2 + 1 < resp.size()) {
            char e = resp[++p2];
            if (e == 'n') out += '\n'; else if (e == 't') out += '\t'; else out += e;
        } else out += ch;
        ++p2;
    }
    return out;
}

static std::string replace_placeholder(const std::string& tmpl, const std::string& input) {
    std::string out = tmpl;
    size_t pos = 0;
    while ((pos = out.find("{input}", pos)) != std::string::npos)
        out.replace(pos, 7, input);
    return out;
}

static std::string trim_lower(std::string s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\n')) s.erase(s.begin());
    while (!s.empty() && (s.back()  == ' ' || s.back()  == '\n')) s.pop_back();
    for (auto& ch : s) ch = (char)std::tolower((unsigned char)ch);
    return s;
}

static double default_score(const std::string& out, const std::string& expected, const std::string&) {
    if (expected.empty()) return 1.0;
    return (trim_lower(out) == trim_lower(expected)) ? 1.0 : 0.0;
}

static void compute_stats(VariantResult& r) {
    r.n = r.scores.size();
    if (r.n == 0) return;
    r.mean_score = std::accumulate(r.scores.begin(), r.scores.end(), 0.0) / (double)r.n;
    double var = 0.0;
    for (double sc : r.scores) { double d = sc - r.mean_score; var += d * d; }
    r.std_dev = (r.n > 1) ? std::sqrt(var / (double)(r.n - 1)) : 0.0;
}

// Regularised incomplete beta function via Lentz continued fraction.
// Used to compute exact two-tailed Welch t-test p-value.
static double ibeta_cf(double a, double b, double x) {
    const double eps = 1e-12, tiny = 1e-300;
    double qab = a + b, qap = a + 1.0, qam = a - 1.0;
    double c = 1.0, d = 1.0 - qab * x / qap;
    if (std::abs(d) < tiny) d = tiny;
    d = 1.0 / d;
    double h = d;
    for (int m = 1; m <= 200; ++m) {
        int m2 = 2 * m;
        double aa = (double)m * (b - (double)m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + aa * d; if (std::abs(d) < tiny) d = tiny;
        c = 1.0 + aa / c; if (std::abs(c) < tiny) c = tiny;
        d = 1.0 / d; h *= d * c;
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1.0 + aa * d; if (std::abs(d) < tiny) d = tiny;
        c = 1.0 + aa / c; if (std::abs(c) < tiny) c = tiny;
        d = 1.0 / d;
        double delta = d * c; h *= delta;
        if (std::abs(delta - 1.0) < eps) break;
    }
    return h;
}

// M_PI is not standard C++ (MSVC hides it unless _USE_MATH_DEFINES is set
// before the first <cmath>), so the header carries its own constant.
static constexpr double kPi = 3.14159265358979323846;

static double log_gamma(double x) {
    // Lanczos approximation (g=7)
    static const double c[] = {
        0.99999999999980993, 676.5203681218851, -1259.1392167224028,
        771.32342877765313, -176.61502916214059, 12.507343278686905,
        -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7
    };
    if (x < 0.5) return std::log(kPi / std::sin(kPi * x)) - log_gamma(1.0 - x);
    x -= 1.0;
    double a = c[0];
    double t = x + 7.5;
    for (int i = 1; i < 9; ++i) a += c[i] / (x + (double)i);
    return 0.5 * std::log(2.0 * kPi) + (x + 0.5) * std::log(t) - t + std::log(a);
}

static double ibeta(double a, double b, double x) {
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;
    double lbeta = log_gamma(a) + log_gamma(b) - log_gamma(a + b);
    double front = std::exp(std::log(x) * a + std::log(1.0 - x) * b - lbeta) / a;
    if (x < (a + 1.0) / (a + b + 2.0))
        return front * ibeta_cf(a, b, x);
    else
        return 1.0 - std::exp(std::log(1.0 - x) * b + std::log(x) * a - lbeta) / b * ibeta_cf(b, a, 1.0 - x);
}

// Exact two-tailed Welch t-test p-value using the incomplete beta function.
static double welch_pvalue(double t_stat, double df) {
    if (df <= 0.0) return 1.0;
    double x = df / (df + t_stat * t_stat);
    return ibeta(df / 2.0, 0.5, x);
}

// Welch-Satterthwaite degrees of freedom.
static double welch_df(double var_a, size_t na, double var_b, size_t nb) {
    double sa = var_a / (double)na, sb = var_b / (double)nb;
    double denom = (sa * sa) / (double)(na - 1) + (sb * sb) / (double)(nb - 1);
    if (denom < 1e-12) return (double)(na + nb - 2);
    return (sa + sb) * (sa + sb) / denom;
}

} // namespace detail_ab

VariantResult run_variant(const Variant& variant,
                           const std::vector<ABSample>& samples,
                           const ABConfig& cfg) {
    auto scorer = cfg.scorer ? cfg.scorer : detail_ab::default_score;
    VariantResult r;
    r.name = variant.name;
    Variant v2 = variant;
    if (v2.api_key.empty()) v2.api_key = cfg.api_key;

    for (auto& s : samples) {
        std::string prompt = detail_ab::replace_placeholder(v2.prompt, s.input);
        std::string call_err;
        std::string output = detail_ab::llm_call(prompt, v2, cfg.timeout_secs, call_err);
        if (!call_err.empty()) { r.error = call_err; break; }
        r.outputs.push_back(output);
        r.scores.push_back(scorer(output, s.expected, s.input));
    }
    detail_ab::compute_stats(r);
    return r;
}

ABResult run_ab_test(const Variant& control,
                      const Variant& treatment,
                      const std::vector<ABSample>& samples,
                      const ABConfig& cfg) {
    ABResult result;
    result.alpha     = cfg.alpha;
    result.control   = run_variant(control,   samples, cfg);
    result.treatment = run_variant(treatment, samples, cfg);

    auto& a = result.control;
    auto& b = result.treatment;

    if (!a.error.empty() || !b.error.empty()) {
        result.winner  = "error";
        result.summary = "Error: " + (a.error.empty() ? b.error : a.error);
        return result;
    }
    if (a.n < 2 || b.n < 2) {
        result.winner  = "tie";
        result.summary = "Not enough samples for significance test.";
        return result;
    }

    double va = a.std_dev * a.std_dev;
    double vb = b.std_dev * b.std_dev;
    double se = std::sqrt(va / (double)a.n + vb / (double)b.n);

    if (se < 1e-12) {
        result.p_value = 1.0;
    } else {
        result.t_statistic = (a.mean_score - b.mean_score) / se;
        double df          = detail_ab::welch_df(va, a.n, vb, b.n);
        result.p_value     = detail_ab::welch_pvalue(result.t_statistic, df);
    }

    double pooled      = std::sqrt((va + vb) / 2.0);
    result.cohens_d    = (pooled > 1e-12) ? std::abs(a.mean_score - b.mean_score) / pooled : 0.0;
    result.significant = result.p_value < cfg.alpha;
    result.winner      = !result.significant ? "tie" :
                         (a.mean_score > b.mean_score) ? "control" : "treatment";

    char buf[256];
    snprintf(buf, sizeof(buf),
             "%s (control=%.3f, treatment=%.3f, p=%.4f, d=%.2f)",
             result.winner == "tie"     ? "No winner" :
             result.winner == "control" ? "Control wins" : "Treatment wins",
             a.mean_score, b.mean_score, result.p_value, result.cohens_d);
    result.summary = buf;
    return result;
}

} // namespace llm
#endif // LLM_AB_IMPLEMENTATION
