#pragma once
// llm-log: single-header structured JSONL logger for LLM API calls
// #define LLM_LOG_IMPLEMENTATION in ONE .cpp before including.

#include <chrono>
#include <ctime>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace llm {

struct LogEntry {
    std::string id;
    std::time_t timestamp     = 0;
    std::string model;
    std::string prompt;
    std::string response;
    size_t      input_tokens  = 0;
    size_t      output_tokens = 0;
    double      cost_usd      = 0.0;
    double      latency_ms    = 0.0;
    bool        success       = true;
    std::string error;
    std::map<std::string, std::string> metadata;
};

struct LogConfig {
    std::string filepath           = "llm_calls.jsonl";
    bool        also_stdout        = false;
    bool        pretty_print       = false;
    size_t      max_prompt_chars   = 0;
    size_t      max_response_chars = 0;
};

class Logger {
public:
    explicit Logger(LogConfig config = {});
    void log(const LogEntry& entry);

    struct ScopedCall {
        ScopedCall(Logger& logger, const std::string& model,
                   const std::string& prompt,
                   std::map<std::string, std::string> metadata = {});
        ~ScopedCall();
        void set_response(const std::string& response);
        void set_tokens(size_t input, size_t output);
        void set_cost(double usd);
        void set_error(const std::string& error);
    private:
        Logger& logger_;
        LogEntry entry_;
        std::chrono::steady_clock::time_point start_;
    };

    struct QueryOptions {
        std::optional<std::string> model_filter;
        std::optional<std::time_t> after;
        std::optional<std::time_t> before;
        std::optional<bool>        success_only;
        size_t                     limit = 100;
    };
    std::vector<LogEntry> query();
    std::vector<LogEntry> query(const QueryOptions& opts);

    struct Summary {
        size_t total_calls;
        size_t successful_calls;
        size_t failed_calls;
        double total_cost_usd;
        size_t total_tokens;
        double avg_latency_ms;
        std::map<std::string, size_t> calls_per_model;
    };
    Summary summarize();

private:
    LogConfig config_;
};

} // namespace llm

#ifdef LLM_LOG_IMPLEMENTATION

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace llm {
namespace detail {

static std::string gen_id() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << dist(rng);
    return oss.str();
}

static std::string json_escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20)  { out += "\\u00"; out += "0123456789abcdef"[c>>4]; out += "0123456789abcdef"[c&0xf]; }
        else                out += static_cast<char>(c);
    }
    return out;
}

static std::string entry_to_json(const LogEntry& e, bool pretty) {
    std::string sp = pretty ? "\n  " : "";
    std::string out = "{";
    out += sp + "\"id\":\"" + json_escape(e.id) + "\",";
    out += sp + "\"timestamp\":" + std::to_string(e.timestamp) + ",";
    out += sp + "\"model\":\"" + json_escape(e.model) + "\",";
    out += sp + "\"prompt\":\"" + json_escape(e.prompt) + "\",";
    out += sp + "\"response\":\"" + json_escape(e.response) + "\",";
    out += sp + "\"input_tokens\":" + std::to_string(e.input_tokens) + ",";
    out += sp + "\"output_tokens\":" + std::to_string(e.output_tokens) + ",";
    out += sp + "\"cost_usd\":" + std::to_string(e.cost_usd) + ",";
    out += sp + "\"latency_ms\":" + std::to_string(e.latency_ms) + ",";
    out += sp + "\"success\":" + (e.success ? "true" : "false") + ",";
    out += sp + "\"error\":\"" + json_escape(e.error) + "\",";
    out += sp + "\"metadata\":{";
    bool first = true;
    for (const auto& kv : e.metadata) {
        if (!first) out += ",";
        out += "\"" + json_escape(kv.first) + "\":\"" + json_escape(kv.second) + "\"";
        first = false;
    }
    out += "}";
    if (pretty) out += "\n";
    out += "}";
    return out;
}

static LogEntry json_to_entry(const std::string& line) {
    LogEntry e;
    auto exstr = [&](const std::string& key) -> std::string {
        auto pos = line.find("\"" + key + "\":\"");
        if (pos == std::string::npos) return "";
        pos += key.size() + 4;
        std::string val;
        while (pos < line.size() && line[pos] != '"') {
            if (line[pos] == '\\' && pos+1 < line.size()) { ++pos; val += line[pos]; }
            else val += line[pos];
            ++pos;
        }
        return val;
    };
    auto exnum = [&](const std::string& key) -> double {
        auto pos = line.find("\"" + key + "\":");
        if (pos == std::string::npos) return 0;
        pos += key.size() + 3;
        std::string num;
        while (pos < line.size() && (std::isdigit((unsigned char)line[pos]) || line[pos]=='.')) num += line[pos++];
        try { return std::stod(num); } catch (...) { return 0; }
    };
    e.id            = exstr("id");
    e.timestamp     = static_cast<std::time_t>(exnum("timestamp"));
    e.model         = exstr("model");
    e.prompt        = exstr("prompt");
    e.response      = exstr("response");
    e.input_tokens  = static_cast<size_t>(exnum("input_tokens"));
    e.output_tokens = static_cast<size_t>(exnum("output_tokens"));
    e.cost_usd      = exnum("cost_usd");
    e.latency_ms    = exnum("latency_ms");
    e.success       = line.find("\"success\":true") != std::string::npos;
    e.error         = exstr("error");
    return e;
}

} // namespace detail

Logger::Logger(LogConfig config) : config_(std::move(config)) {}

void Logger::log(const LogEntry& in_entry) {
    LogEntry entry = in_entry;
    if (entry.id.empty()) entry.id = detail::gen_id();
    if (entry.timestamp == 0) entry.timestamp = std::time(nullptr);
    if (config_.max_prompt_chars > 0 && entry.prompt.size() > config_.max_prompt_chars)
        entry.prompt = entry.prompt.substr(0, config_.max_prompt_chars) + "...";
    if (config_.max_response_chars > 0 && entry.response.size() > config_.max_response_chars)
        entry.response = entry.response.substr(0, config_.max_response_chars) + "...";
    std::string json = detail::entry_to_json(entry, config_.pretty_print);
    if (!config_.filepath.empty()) {
        std::ofstream f(config_.filepath, std::ios::app);
        if (f) f << json << "\n";
    }
    if (config_.also_stdout || config_.filepath.empty())
        std::cout << json << "\n";
}

Logger::ScopedCall::ScopedCall(Logger& logger, const std::string& model,
                                const std::string& prompt,
                                std::map<std::string, std::string> metadata)
    : logger_(logger), start_(std::chrono::steady_clock::now()) {
    entry_.model    = model;
    entry_.prompt   = prompt;
    entry_.metadata = std::move(metadata);
    entry_.success  = true;
}

Logger::ScopedCall::~ScopedCall() {
    entry_.latency_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start_).count();
    logger_.log(entry_);
}

void Logger::ScopedCall::set_response(const std::string& r) { entry_.response = r; }
void Logger::ScopedCall::set_tokens(size_t in, size_t out) { entry_.input_tokens = in; entry_.output_tokens = out; }
void Logger::ScopedCall::set_cost(double usd) { entry_.cost_usd = usd; }
void Logger::ScopedCall::set_error(const std::string& err) { entry_.error = err; entry_.success = false; }

std::vector<LogEntry> Logger::query() { return query(QueryOptions{}); }

std::vector<LogEntry> Logger::query(const QueryOptions& opts) {
    std::vector<LogEntry> results;
    std::ifstream f(config_.filepath);
    if (!f) return results;
    std::string line;
    while (std::getline(f, line) && results.size() < opts.limit) {
        if (line.empty()) continue;
        LogEntry e = detail::json_to_entry(line);
        if (opts.model_filter && e.model != *opts.model_filter) continue;
        if (opts.after  && e.timestamp <= *opts.after)  continue;
        if (opts.before && e.timestamp >= *opts.before) continue;
        if (opts.success_only && *opts.success_only && !e.success) continue;
        results.push_back(std::move(e));
    }
    return results;
}

Logger::Summary Logger::summarize() {
    Summary s{};
    std::ifstream f(config_.filepath);
    if (!f) return s;
    std::string line;
    double total_lat = 0;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        LogEntry e = detail::json_to_entry(line);
        ++s.total_calls;
        if (e.success) ++s.successful_calls; else ++s.failed_calls;
        s.total_cost_usd += e.cost_usd;
        s.total_tokens   += e.input_tokens + e.output_tokens;
        total_lat        += e.latency_ms;
        ++s.calls_per_model[e.model];
    }
    s.avg_latency_ms = s.total_calls > 0 ? total_lat / s.total_calls : 0;
    return s;
}

} // namespace llm
#endif // LLM_LOG_IMPLEMENTATION