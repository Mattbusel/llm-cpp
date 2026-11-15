#pragma once
#define NOMINMAX

// llm_compress.hpp -- Zero-dependency single-header C++ conversation compressor.
// Strategies: TruncateHead, TruncateTail, TruncateSmart, SlidingWindow, Summarize.
// compress_messages() reduces token usage while preserving context.
//
// USAGE:
//   #define LLM_COMPRESS_IMPLEMENTATION  (in exactly one .cpp)
//   #include <cstdint>
#include "llm_compress.hpp"
//
// Requires: libcurl (only for Summarize strategy)

#include <climits>
#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace llm {

struct CompressMessage {
    std::string role;    // "system", "user", "assistant"
    std::string content;
};

// ---------------------------------------------------------------------------
// Strategies
// ---------------------------------------------------------------------------

/// Remove oldest messages (after system prompt) until under budget.
struct TruncateHead {};

/// Remove newest non-system messages until under budget.
struct TruncateTail {};

/// Remove messages with lowest information density (shortest non-system messages first).
struct TruncateSmart {};

/// Keep a sliding window of the N most recent turns (preserves system prompt).
struct SlidingWindow {
    size_t max_turns = 10; // each turn = one user+assistant pair
};

/// Summarize removed context via LLM API call, inject summary as system context.
struct Summarize {
    std::string api_key;
    std::string model = "gpt-4o-mini";
};

using CompressStrategy = std::variant<
    TruncateHead,
    TruncateTail,
    TruncateSmart,
    SlidingWindow,
    Summarize
>;

// ---------------------------------------------------------------------------
// Config & result
// ---------------------------------------------------------------------------

struct CompressConfig {
    CompressStrategy strategy    = TruncateHead{};
    size_t           token_budget = 4096; // estimated as chars/4
    bool             preserve_system = true; // never remove role=="system" messages
};

struct CompressResult {
    std::vector<CompressMessage> messages;   // compressed message list
    size_t tokens_before;
    size_t tokens_after;
    size_t messages_removed;
};

/// Estimate token count for a message list (chars / 4).
size_t estimate_tokens(const std::vector<CompressMessage>& messages);

/// Compress messages to fit within config.token_budget.
CompressResult compress_messages(const std::vector<CompressMessage>& messages,
                                 const CompressConfig& config);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_COMPRESS_IMPLEMENTATION

#include <algorithm>
#include <numeric>
#include <sstream>

#ifdef LLM_COMPRESS_SUMMARIZE
#include <curl/curl.h>
#endif

namespace llm {
namespace detail_compress {

static size_t msg_tokens(const CompressMessage& m) {
    return (m.content.size() + 3) / 4;
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

#ifdef LLM_COMPRESS_SUMMARIZE
static size_t wcb(char* p, size_t s, size_t n, void* ud) {
    static_cast<std::string*>(ud)->append(p, s * n); return s * n;
}

static std::string summarize_via_llm(const std::vector<CompressMessage>& removed,
                                      const Summarize& cfg) {
    std::string context;
    for (const auto& m : removed)
        context += m.role + ": " + m.content + "\n";
    std::string prompt = "Summarize this conversation briefly:\\n" + context;

    std::string body = "{\"model\":\"" + jesc(cfg.model) + "\","
                       "\"max_tokens\":256,"
                       "\"messages\":[{\"role\":\"user\",\"content\":\""
                       + jesc(prompt) + "\"}]}";

    CURL* curl = curl_easy_init();
    if (!curl) return "[summary unavailable]";
    curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    hdrs = curl_slist_append(hdrs, ("Authorization: Bearer " + cfg.api_key).c_str());
    std::string resp;
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    hdrs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wcb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       30L);
    curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    // extract content
    auto p = resp.find("\"content\":\"");
    if (p == std::string::npos) return "[summary unavailable]";
    p += 11;
    std::string val;
    while (p < resp.size() && resp[p] != '"') {
        if (resp[p] == '\\' && p + 1 < resp.size()) { ++p; val += resp[p]; }
        else val += resp[p];
        ++p;
    }
    return val;
}
#endif

} // namespace detail_compress

size_t estimate_tokens(const std::vector<CompressMessage>& messages) {
    size_t t = 0;
    for (const auto& m : messages) t += detail_compress::msg_tokens(m);
    return t;
}

CompressResult compress_messages(const std::vector<CompressMessage>& messages,
                                  const CompressConfig& config) {
    CompressResult result;
    result.tokens_before   = estimate_tokens(messages);
    result.messages_removed = 0;

    auto working = messages;

    auto under_budget = [&]() {
        return estimate_tokens(working) <= config.token_budget;
    };

    auto is_system = [](const CompressMessage& m) {
        return m.role == "system";
    };

    if (std::holds_alternative<TruncateHead>(config.strategy)) {
        // Remove from front (after system) until under budget
        while (!under_budget() && working.size() > 1) {
            size_t idx = 0;
            if (config.preserve_system && !working.empty() && is_system(working[0]))
                idx = 1;
            if (idx >= working.size()) break;
            working.erase(working.begin() + static_cast<long>(idx));
            ++result.messages_removed;
        }

    } else if (std::holds_alternative<TruncateTail>(config.strategy)) {
        // Remove from back until under budget
        while (!under_budget() && working.size() > 1) {
            size_t idx = working.size() - 1;
            if (config.preserve_system && is_system(working[idx])) {
                if (idx == 0) break;
                --idx;
            }
            working.erase(working.begin() + static_cast<long>(idx));
            ++result.messages_removed;
        }

    } else if (std::holds_alternative<TruncateSmart>(config.strategy)) {
        // Remove shortest non-system messages first (lowest info density)
        while (!under_budget() && working.size() > 1) {
            size_t min_idx  = std::string::npos;
            size_t min_size = SIZE_MAX;
            for (size_t i = 0; i < working.size(); ++i) {
                if (config.preserve_system && is_system(working[i])) continue;
                if (working[i].content.size() < min_size) {
                    min_size = working[i].content.size();
                    min_idx  = i;
                }
            }
            if (min_idx == std::string::npos) break;
            working.erase(working.begin() + static_cast<long>(min_idx));
            ++result.messages_removed;
        }

    } else if (auto* sw = std::get_if<SlidingWindow>(&config.strategy)) {
        // Keep system + last max_turns*2 messages
        size_t sys_count = 0;
        for (const auto& m : working) if (is_system(m)) ++sys_count;
        size_t keep_count = sys_count + sw->max_turns * 2;
        if (working.size() > keep_count) {
            // Separate system and non-system
            std::vector<CompressMessage> sys_msgs, other_msgs;
            for (const auto& m : working) {
                if (is_system(m)) sys_msgs.push_back(m);
                else              other_msgs.push_back(m);
            }
            size_t other_keep = sw->max_turns * 2;
            if (other_msgs.size() > other_keep) {
                result.messages_removed += other_msgs.size() - other_keep;
                other_msgs.erase(other_msgs.begin(),
                                  other_msgs.begin() + static_cast<long>(other_msgs.size() - other_keep));
            }
            working = sys_msgs;
            working.insert(working.end(), other_msgs.begin(), other_msgs.end());
        }

    } else if (auto* summ = std::get_if<Summarize>(&config.strategy)) {
        (void)summ;
        // Collect messages to remove (TruncateHead style), then summarize them
        std::vector<CompressMessage> removed;
        while (!under_budget() && working.size() > 1) {
            size_t idx = 0;
            if (config.preserve_system && !working.empty() && is_system(working[0]))
                idx = 1;
            if (idx >= working.size()) break;
            removed.push_back(working[idx]);
            working.erase(working.begin() + static_cast<long>(idx));
            ++result.messages_removed;
        }
        if (!removed.empty()) {
#ifdef LLM_COMPRESS_SUMMARIZE
            std::string summary = detail_compress::summarize_via_llm(removed, *summ);
#else
            std::string summary = "[Summarized " + std::to_string(removed.size()) +
                                   " messages — define LLM_COMPRESS_SUMMARIZE and link libcurl to enable]";
#endif
            // Inject summary as a system message after any existing system messages
            size_t inject_at = 0;
            while (inject_at < working.size() && is_system(working[inject_at])) ++inject_at;
            working.insert(working.begin() + static_cast<long>(inject_at),
                           {"system", "Previous context summary: " + summary});
        }
    }

    result.messages    = working;
    result.tokens_after = estimate_tokens(working);
    return result;
}

} // namespace llm
#endif // LLM_COMPRESS_IMPLEMENTATION