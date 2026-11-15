#pragma once
#define NOMINMAX

// llm_batch.hpp -- Zero-dependency single-header C++ batch LLM processor.
// JSONL batch processing, thread pool, checkpointing/resumability, rate limiting.
// process_file() reads prompts.jsonl, writes results.jsonl.
// process_batch() processes in-memory prompt list.
//
// USAGE:
//   #define LLM_BATCH_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_batch.hpp"
//
// Requires: libcurl

#include <functional>
#include <string>
#include <vector>

namespace llm {

struct BatchItem {
    std::string id;
    std::string prompt;
    std::string system_prompt; // optional per-item system prompt
};

struct BatchResult {
    std::string id;
    std::string prompt;
    std::string response;
    bool        success   = true;
    std::string error;
    double      latency_ms = 0.0;
};

struct BatchConfig {
    std::string api_key;
    std::string model         = "gpt-4o-mini";
    std::string api_url       = "https://api.openai.com/v1/chat/completions";
    int         max_tokens    = 1024;
    double      temperature   = 0.0;
    size_t      num_threads   = 4;        // worker thread count
    double      rate_limit_rps = 5.0;    // max requests per second (0 = unlimited)
    std::string checkpoint_path;          // if set, resume from checkpoint
    bool        verbose       = false;

    // Progress callback: called after each item completes (thread-safe)
    std::function<void(const BatchResult&, size_t done, size_t total)> on_progress;
};

/// Process an in-memory list of items.
std::vector<BatchResult> process_batch(const std::vector<BatchItem>& items,
                                        const BatchConfig& config);

/// Read prompts from a JSONL file (fields: "id", "prompt", optionally "system"),
/// write results to output_path as JSONL.
/// Returns number of successfully processed items.
size_t process_file(const std::string& input_path,
                    const std::string& output_path,
                    const BatchConfig& config);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_BATCH_IMPLEMENTATION

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <curl/curl.h>

namespace llm {
namespace detail_batch {

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
    static_cast<std::string*>(ud)->append(p, s * n); return s * n;
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
            if (e == 'n') v += '\n'; else if (e == 't') v += '\t'; else v += e;
        } else v += j[p];
        ++p;
    }
    return v;
}

static BatchResult do_request(const BatchItem& item, const BatchConfig& cfg) {
    BatchResult result;
    result.id     = item.id;
    result.prompt = item.prompt;

    auto t0 = std::chrono::steady_clock::now();

    std::string msgs = "[";
    if (!item.system_prompt.empty())
        msgs += "{\"role\":\"system\",\"content\":\"" + jesc(item.system_prompt) + "\"},";
    msgs += "{\"role\":\"user\",\"content\":\"" + jesc(item.prompt) + "\"}]";

    std::string body = "{\"model\":\"" + jesc(cfg.model) + "\","
                       "\"max_tokens\":" + std::to_string(cfg.max_tokens) + ","
                       "\"temperature\":" + std::to_string(cfg.temperature) + ","
                       "\"messages\":" + msgs + "}";

    CurlH c; if (!c.ok()) { result.success = false; result.error = "curl init failed"; return result; }
    CurlSl h;
    h.append("Content-Type: application/json");
    h.append(("Authorization: Bearer " + cfg.api_key).c_str());
    std::string resp;
    curl_easy_setopt(c.h, CURLOPT_URL,            cfg.api_url.c_str());
    curl_easy_setopt(c.h, CURLOPT_HTTPHEADER,     h.l);
    curl_easy_setopt(c.h, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(c.h, CURLOPT_WRITEFUNCTION,  wcb);
    curl_easy_setopt(c.h, CURLOPT_WRITEDATA,      &resp);
    curl_easy_setopt(c.h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,        120L);
    CURLcode rc = curl_easy_perform(c.h);

    result.latency_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    if (rc != CURLE_OK) {
        result.success = false;
        result.error   = curl_easy_strerror(rc);
        return result;
    }

    // parse response
    auto p = resp.find("\"message\"");
    if (p == std::string::npos) p = resp.rfind("\"content\"");
    if (p != std::string::npos) {
        result.response = jstr(resp.substr(p), "content");
    }
    if (result.response.empty()) {
        // check for error
        auto ep = resp.find("\"error\"");
        if (ep != std::string::npos) {
            result.success = false;
            result.error   = jstr(resp.substr(ep), "message");
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Rate limiter: token bucket
// ---------------------------------------------------------------------------
struct RateLimiter {
    double rps;
    std::chrono::steady_clock::time_point last_time;
    std::mutex mtx;
    double tokens = 0.0;

    explicit RateLimiter(double rps_) : rps(rps_), last_time(std::chrono::steady_clock::now()) {}

    void acquire() {
        if (rps <= 0.0) return;
        std::unique_lock<std::mutex> lk(mtx);
        while (true) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - last_time).count();
            last_time = now;
            tokens += elapsed * rps;
            if (tokens > rps) tokens = rps; // cap bucket
            if (tokens >= 1.0) { tokens -= 1.0; return; }
            double wait_ms = (1.0 - tokens) / rps * 1000.0;
            lk.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(
                static_cast<long long>(wait_ms * 1000.0)));
            lk.lock();
        }
    }
};

// ---------------------------------------------------------------------------
// Checkpoint (simple JSONL of completed IDs)
// ---------------------------------------------------------------------------
static std::set<std::string> load_checkpoint(const std::string& path) {
    std::set<std::string> done;
    if (path.empty()) return done;
    std::ifstream f(path);
    if (!f) return done;
    std::string line;
    while (std::getline(f, line)) {
        auto id = jstr(line, "id");
        if (!id.empty()) done.insert(id);
    }
    return done;
}

static void append_checkpoint(const std::string& path, const BatchResult& r) {
    if (path.empty()) return;
    std::ofstream f(path, std::ios::app);
    if (!f) return;
    f << "{\"id\":\"" << jesc(r.id) << "\","
      << "\"success\":" << (r.success ? "true" : "false") << "}\n";
}

} // namespace detail_batch

// ---------------------------------------------------------------------------

std::vector<BatchResult> process_batch(const std::vector<BatchItem>& items,
                                        const BatchConfig& config) {
    if (items.empty()) return {};

    // Load checkpoint
    auto done_ids = detail_batch::load_checkpoint(config.checkpoint_path);

    std::vector<BatchResult> results(items.size());
    // Pre-fill skipped items
    for (size_t i = 0; i < items.size(); ++i) {
        results[i].id     = items[i].id;
        results[i].prompt = items[i].prompt;
    }

    std::queue<size_t>      work_queue;
    std::mutex              queue_mtx;
    std::atomic<size_t>     done_count{0};
    std::mutex              results_mtx;
    detail_batch::RateLimiter limiter(config.rate_limit_rps);

    for (size_t i = 0; i < items.size(); ++i) {
        if (done_ids.count(items[i].id) == 0)
            work_queue.push(i);
        else {
            results[i].response = "[skipped - checkpoint]";
            ++done_count;
        }
    }

    size_t total = items.size();
    size_t num_threads = std::min(config.num_threads, items.size());

    auto worker = [&]() {
        while (true) {
            size_t idx;
            {
                std::unique_lock<std::mutex> lk(queue_mtx);
                if (work_queue.empty()) return;
                idx = work_queue.front();
                work_queue.pop();
            }
            limiter.acquire();
            BatchResult r = detail_batch::do_request(items[idx], config);
            {
                std::unique_lock<std::mutex> lk(results_mtx);
                results[idx] = r;
                detail_batch::append_checkpoint(config.checkpoint_path, r);
            }
            size_t d = ++done_count;
            if (config.on_progress) config.on_progress(r, d, total);
            if (config.verbose) {
                fprintf(stderr, "[batch] %zu/%zu id=%s %s\n",
                        d, total, r.id.c_str(), r.success ? "OK" : r.error.c_str());
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads) t.join();

    return results;
}

size_t process_file(const std::string& input_path,
                    const std::string& output_path,
                    const BatchConfig& config) {
    std::ifstream fin(input_path);
    if (!fin) throw std::runtime_error("Cannot open: " + input_path);

    std::vector<BatchItem> items;
    std::string line;
    size_t auto_id = 0;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        BatchItem item;
        item.id           = detail_batch::jstr(line, "id");
        item.prompt       = detail_batch::jstr(line, "prompt");
        item.system_prompt = detail_batch::jstr(line, "system");
        if (item.id.empty()) item.id = std::to_string(++auto_id);
        if (!item.prompt.empty()) items.push_back(item);
    }

    auto results = process_batch(items, config);

    std::ofstream fout(output_path);
    if (!fout) throw std::runtime_error("Cannot write: " + output_path);
    size_t success_count = 0;
    for (const auto& r : results) {
        fout << "{\"id\":\"" << detail_batch::jesc(r.id) << "\","
             << "\"prompt\":\"" << detail_batch::jesc(r.prompt) << "\","
             << "\"response\":\"" << detail_batch::jesc(r.response) << "\","
             << "\"success\":" << (r.success ? "true" : "false") << ","
             << "\"latency_ms\":" << r.latency_ms;
        if (!r.error.empty())
            fout << ",\"error\":\"" << detail_batch::jesc(r.error) << "\"";
        fout << "}\n";
        if (r.success) ++success_count;
    }
    return success_count;
}

} // namespace llm
#endif // LLM_BATCH_IMPLEMENTATION
