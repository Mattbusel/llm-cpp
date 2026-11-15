#pragma once
#define NOMINMAX

// llm_finetune.hpp -- Zero-dependency single-header C++ fine-tuning job manager.
// Upload training data, start jobs, poll status, list/delete fine-tuned models.
//
// USAGE:
//   #define LLM_FINETUNE_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_finetune.hpp"
//
// Requires: libcurl

#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace llm {

// ---------------------------------------------------------------------------
// Training data
// ---------------------------------------------------------------------------

struct TrainingExample {
    std::string system_prompt;
    std::string user_message;
    std::string assistant_response;
};

/// Write TrainingExample list to a JSONL file.
void write_training_jsonl(const std::vector<TrainingExample>& examples,
                           const std::string& filepath);

// ---------------------------------------------------------------------------
// File upload
// ---------------------------------------------------------------------------

struct UploadResult {
    std::string file_id;
    std::string filename;
    size_t      bytes   = 0;
    std::string purpose;
    std::string status;
};

UploadResult upload_training_file(const std::string& jsonl_filepath,
                                   const std::string& api_key);

UploadResult upload_examples(const std::vector<TrainingExample>& examples,
                              const std::string& api_key);

// ---------------------------------------------------------------------------
// Fine-tune jobs
// ---------------------------------------------------------------------------

struct FinetuneConfig {
    std::string api_key;
    std::string base_model               = "gpt-4o-mini-2024-07-18";
    int         n_epochs                  = 3;   // -1 = auto
    int         batch_size                = -1;  // -1 = auto
    double      learning_rate_multiplier  = -1.0;// -1 = auto
    std::string suffix;
};

struct FinetuneJob {
    std::string id;
    std::string status;           // validating_files, queued, running, succeeded, failed, cancelled
    std::string model;
    std::string fine_tuned_model;
    std::string training_file;
    std::time_t created_at  = 0;
    std::time_t finished_at = 0;
    int         trained_tokens = 0;
    std::string error;
};

FinetuneJob create_job(const std::string& training_file_id,
                        const FinetuneConfig& config);

FinetuneJob get_job(const std::string& job_id, const std::string& api_key);

std::vector<FinetuneJob> list_jobs(const std::string& api_key, int limit = 20);

FinetuneJob cancel_job(const std::string& job_id, const std::string& api_key);

/// Poll until job completes or fails; calls on_status each interval.
/// max_wait_seconds: 0 = default 24h cap, otherwise hard deadline.
FinetuneJob wait_for_completion(
    const std::string& job_id,
    const std::string& api_key,
    int poll_interval_seconds = 30,
    std::function<void(const FinetuneJob&)> on_status = nullptr,
    int max_wait_seconds = 0);

// ---------------------------------------------------------------------------
// Model management
// ---------------------------------------------------------------------------

struct FinetunedModel {
    std::string id;
    std::string base_model;
    std::time_t created_at = 0;
};

std::vector<FinetunedModel> list_models(const std::string& api_key);
void delete_model(const std::string& model_id, const std::string& api_key);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_FINETUNE_IMPLEMENTATION

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <curl/curl.h>

namespace llm {
namespace detail_ft {

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

static std::string http_get(const std::string& url, const std::string& key) {
    CurlH c; if (!c.ok()) return {};
    CurlSl h;
    h.append(("Authorization: Bearer " + key).c_str());
    std::string resp;
    curl_easy_setopt(c.h, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(c.h, CURLOPT_HTTPHEADER,     h.l);
    curl_easy_setopt(c.h, CURLOPT_WRITEFUNCTION,  wcb);
    curl_easy_setopt(c.h, CURLOPT_WRITEDATA,      &resp);
    curl_easy_setopt(c.h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,        30L);
    curl_easy_perform(c.h);
    return resp;
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
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,        30L);
    curl_easy_perform(c.h);
    return resp;
}

static std::string http_delete(const std::string& url, const std::string& key) {
    CurlH c; if (!c.ok()) return {};
    CurlSl h;
    h.append(("Authorization: Bearer " + key).c_str());
    std::string resp;
    curl_easy_setopt(c.h, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(c.h, CURLOPT_HTTPHEADER,     h.l);
    curl_easy_setopt(c.h, CURLOPT_CUSTOMREQUEST,  "DELETE");
    curl_easy_setopt(c.h, CURLOPT_WRITEFUNCTION,  wcb);
    curl_easy_setopt(c.h, CURLOPT_WRITEDATA,      &resp);
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,        30L);
    curl_easy_perform(c.h);
    return resp;
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
        if (j[p] == '\\' && p + 1 < j.size()) { char e = j[++p]; if(e=='n')v+='\n';else v+=e; }
        else v += j[p];
        ++p;
    }
    return v;
}

static long long jint(const std::string& j, const std::string& k) {
    std::string pat = "\"" + k + "\"";
    auto p = j.find(pat);
    if (p == std::string::npos) return 0;
    p += pat.size();
    while (p < j.size() && (j[p] == ':' || j[p] == ' ')) ++p;
    if (p >= j.size()) return 0;
    if (j[p] == '"') return 0; // string, not number
    char* end = nullptr;
    long long v = std::strtoll(j.c_str() + p, &end, 10);
    return (end != j.c_str() + p) ? v : 0;
}

static FinetuneJob parse_job(const std::string& j) {
    FinetuneJob job;
    job.id               = jstr(j, "id");
    job.status           = jstr(j, "status");
    job.model            = jstr(j, "model");
    job.fine_tuned_model = jstr(j, "fine_tuned_model");
    job.training_file    = jstr(j, "training_file");
    job.created_at       = static_cast<std::time_t>(jint(j, "created_at"));
    job.finished_at      = static_cast<std::time_t>(jint(j, "finished_at"));
    job.trained_tokens   = static_cast<int>(jint(j, "trained_tokens"));
    // error is nested: "error": {"message": "..."}
    auto ep = j.find("\"error\"");
    if (ep != std::string::npos) job.error = jstr(j.substr(ep), "message");
    return job;
}

// Multipart upload for JSONL file
static std::string upload_file_multipart(const std::string& content,
                                          const std::string& filename,
                                          const std::string& key) {
    static const std::string BOUNDARY = "----LLMFinetuneBoundary8a2b4c6d";
    std::string body;
    // file part
    body += "--" + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n";
    body += "Content-Type: application/json\r\n\r\n";
    body += content + "\r\n";
    // purpose part
    body += "--" + BOUNDARY + "\r\n";
    body += "Content-Disposition: form-data; name=\"purpose\"\r\n\r\n";
    body += "fine-tune\r\n";
    body += "--" + BOUNDARY + "--\r\n";

    CurlH c; if (!c.ok()) return {};
    CurlSl h;
    h.append(("Authorization: Bearer " + key).c_str());
    h.append(("Content-Type: multipart/form-data; boundary=" + BOUNDARY).c_str());
    std::string resp;
    curl_easy_setopt(c.h, CURLOPT_URL,           "https://api.openai.com/v1/files");
    curl_easy_setopt(c.h, CURLOPT_HTTPHEADER,    h.l);
    curl_easy_setopt(c.h, CURLOPT_POSTFIELDS,    body.c_str());
    curl_easy_setopt(c.h, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(c.h, CURLOPT_WRITEFUNCTION, wcb);
    curl_easy_setopt(c.h, CURLOPT_WRITEDATA,     &resp);
    curl_easy_setopt(c.h, CURLOPT_TIMEOUT,       120L);
    curl_easy_perform(c.h);
    return resp;
}

} // namespace detail_ft

// ---------------------------------------------------------------------------

void write_training_jsonl(const std::vector<TrainingExample>& examples,
                           const std::string& filepath) {
    std::ofstream f(filepath);
    if (!f) throw std::runtime_error("Cannot write: " + filepath);
    for (const auto& ex : examples) {
        f << "{\"messages\":["
          << "{\"role\":\"system\",\"content\":\"" << detail_ft::jesc(ex.system_prompt) << "\"},"
          << "{\"role\":\"user\",\"content\":\"" << detail_ft::jesc(ex.user_message) << "\"},"
          << "{\"role\":\"assistant\",\"content\":\"" << detail_ft::jesc(ex.assistant_response) << "\"}"
          << "]}\n";
    }
}

UploadResult upload_training_file(const std::string& jsonl_filepath,
                                   const std::string& api_key) {
    std::ifstream f(jsonl_filepath);
    if (!f) throw std::runtime_error("Cannot open: " + jsonl_filepath);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    auto slash = jsonl_filepath.find_last_of("/\\");
    std::string filename = (slash == std::string::npos) ? jsonl_filepath : jsonl_filepath.substr(slash + 1);
    std::string resp = detail_ft::upload_file_multipart(content, filename, api_key);

    UploadResult r;
    r.file_id  = detail_ft::jstr(resp, "id");
    r.filename = detail_ft::jstr(resp, "filename");
    r.purpose  = detail_ft::jstr(resp, "purpose");
    r.status   = detail_ft::jstr(resp, "status");
    r.bytes    = static_cast<size_t>(detail_ft::jint(resp, "bytes"));
    return r;
}

UploadResult upload_examples(const std::vector<TrainingExample>& examples,
                              const std::string& api_key) {
    std::ostringstream ss;
    for (const auto& ex : examples) {
        ss << "{\"messages\":["
           << "{\"role\":\"system\",\"content\":\"" << detail_ft::jesc(ex.system_prompt) << "\"},"
           << "{\"role\":\"user\",\"content\":\"" << detail_ft::jesc(ex.user_message) << "\"},"
           << "{\"role\":\"assistant\",\"content\":\"" << detail_ft::jesc(ex.assistant_response) << "\"}"
           << "]}\n";
    }
    std::string resp = detail_ft::upload_file_multipart(ss.str(), "training.jsonl", api_key);
    UploadResult r;
    r.file_id  = detail_ft::jstr(resp, "id");
    r.filename = detail_ft::jstr(resp, "filename");
    r.purpose  = detail_ft::jstr(resp, "purpose");
    r.status   = detail_ft::jstr(resp, "status");
    r.bytes    = static_cast<size_t>(detail_ft::jint(resp, "bytes"));
    return r;
}

FinetuneJob create_job(const std::string& training_file_id,
                        const FinetuneConfig& config) {
    std::string body = "{\"model\":\"" + detail_ft::jesc(config.base_model) + "\","
                       "\"training_file\":\"" + detail_ft::jesc(training_file_id) + "\"";
    {
        std::string hp;
        if (config.n_epochs > 0)
            hp += "\"n_epochs\":" + std::to_string(config.n_epochs);
        if (config.batch_size > 0) {
            if (!hp.empty()) hp += ',';
            hp += "\"batch_size\":" + std::to_string(config.batch_size);
        }
        if (config.learning_rate_multiplier > 0.0) {
            if (!hp.empty()) hp += ',';
            hp += "\"learning_rate_multiplier\":" + std::to_string(config.learning_rate_multiplier);
        }
        if (!hp.empty()) body += ",\"hyperparameters\":{" + hp + "}";
    }
    if (!config.suffix.empty())
        body += ",\"suffix\":\"" + detail_ft::jesc(config.suffix) + "\"";
    body += "}";
    std::string resp = detail_ft::http_post("https://api.openai.com/v1/fine_tuning/jobs",
                                             body, config.api_key);
    return detail_ft::parse_job(resp);
}

FinetuneJob get_job(const std::string& job_id, const std::string& api_key) {
    std::string resp = detail_ft::http_get(
        "https://api.openai.com/v1/fine_tuning/jobs/" + job_id, api_key);
    return detail_ft::parse_job(resp);
}

std::vector<FinetuneJob> list_jobs(const std::string& api_key, int limit) {
    std::string resp = detail_ft::http_get(
        "https://api.openai.com/v1/fine_tuning/jobs?limit=" + std::to_string(limit), api_key);
    std::vector<FinetuneJob> jobs;
    size_t p = 0;
    while (true) {
        auto found = resp.find("\"id\":\"ftjob-", p);
        if (found == std::string::npos) break;
        // find the enclosing object
        auto ob = resp.rfind('{', found);
        if (ob == std::string::npos) break;
        // find matching }
        int depth = 0; size_t cb = ob;
        for (; cb < resp.size(); ++cb) {
            if (resp[cb] == '{') ++depth;
            else if (resp[cb] == '}') { --depth; if (depth == 0) break; }
        }
        jobs.push_back(detail_ft::parse_job(resp.substr(ob, cb - ob + 1)));
        p = cb + 1;
    }
    return jobs;
}

FinetuneJob cancel_job(const std::string& job_id, const std::string& api_key) {
    std::string resp = detail_ft::http_post(
        "https://api.openai.com/v1/fine_tuning/jobs/" + job_id + "/cancel",
        "{}", api_key);
    return detail_ft::parse_job(resp);
}

FinetuneJob wait_for_completion(const std::string& job_id,
                                 const std::string& api_key,
                                 int poll_interval_seconds,
                                 std::function<void(const FinetuneJob&)> on_status,
                                 int max_wait_seconds) {
    auto deadline = std::chrono::steady_clock::now()
                    + std::chrono::seconds(max_wait_seconds > 0 ? max_wait_seconds : 86400);
    while (std::chrono::steady_clock::now() < deadline) {
        FinetuneJob job = get_job(job_id, api_key);
        if (on_status) on_status(job);
        if (job.status == "succeeded" || job.status == "failed" || job.status == "cancelled")
            return job;
        std::this_thread::sleep_for(std::chrono::seconds(poll_interval_seconds));
    }
    FinetuneJob timed_out = get_job(job_id, api_key);
    timed_out.error = "wait_for_completion: timed out after "
                      + std::to_string(max_wait_seconds) + " seconds";
    return timed_out;
}

std::vector<FinetunedModel> list_models(const std::string& api_key) {
    std::string resp = detail_ft::http_get("https://api.openai.com/v1/models", api_key);
    std::vector<FinetunedModel> models;
    size_t p = 0;
    while (true) {
        auto found = resp.find("\"ft:openai-", p);
        if (found == std::string::npos) found = resp.find("\"ftjob-", p);
        if (found == std::string::npos) {
            // fallback: any fine-tuned model pattern (gpt-*/ft-*)
            found = resp.find("\"gpt-", p);
            if (found != std::string::npos) {
                // only pick up if it contains ":ft-" indicating a fine-tuned variant
                auto colon = resp.find(":ft-", found);
                if (colon == std::string::npos || colon > found + 60) found = std::string::npos;
            }
        }
        if (found == std::string::npos) break;
        auto ob = resp.rfind('{', found);
        if (ob == std::string::npos) break;
        int depth = 0; size_t cb = ob;
        for (; cb < resp.size(); ++cb) {
            if (resp[cb] == '{') ++depth;
            else if (resp[cb] == '}') { --depth; if (depth == 0) break; }
        }
        std::string chunk = resp.substr(ob, cb - ob + 1);
        FinetunedModel m;
        m.id         = detail_ft::jstr(chunk, "id");
        m.created_at = static_cast<std::time_t>(detail_ft::jint(chunk, "created"));
        models.push_back(m);
        p = cb + 1;
    }
    return models;
}

void delete_model(const std::string& model_id, const std::string& api_key) {
    detail_ft::http_delete("https://api.openai.com/v1/models/" + model_id, api_key);
}

} // namespace llm
#endif // LLM_FINETUNE_IMPLEMENTATION
