#pragma once
#define NOMINMAX

// llm_mock.hpp -- Zero-dependency single-header C++ mock LLM for testing.
// ScriptedResponses, PatternResponses, RandomResponses, EchoResponse strategies.
// Streaming simulation with configurable delay. install_mock()/uninstall_mock().
//
// USAGE:
//   #define LLM_MOCK_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_mock.hpp"
//
// No external dependencies required.

#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace llm {

// ---------------------------------------------------------------------------
// Response strategies
// ---------------------------------------------------------------------------

/// Returns responses from a fixed list in order (wraps around).
struct ScriptedResponses {
    std::vector<std::string> responses;
    size_t                   index = 0;
};

/// Returns the first response whose pattern appears in the prompt (substring match).
struct PatternResponses {
    std::vector<std::pair<std::string, std::string>> patterns; // {pattern, response}
    std::string fallback = "[no pattern matched]";
};

/// Returns a random response from a fixed list.
struct RandomResponses {
    std::vector<std::string> responses;
    unsigned int             seed = 42;
};

/// Echoes the prompt back (optionally with a prefix).
struct EchoResponse {
    std::string prefix = "Echo: ";
};

using MockStrategy = std::variant<
    ScriptedResponses,
    PatternResponses,
    RandomResponses,
    EchoResponse
>;

// ---------------------------------------------------------------------------
// Mock config
// ---------------------------------------------------------------------------

struct MockConfig {
    MockStrategy strategy    = EchoResponse{};
    double       latency_ms  = 0.0;   // simulated delay per call
    bool         stream_sim  = false; // simulate streaming (token-by-token)
    size_t       tokens_per_chunk = 3; // words per streaming chunk
};

using StreamCallback = std::function<void(const std::string& delta)>;

// ---------------------------------------------------------------------------
// Mock interface
// ---------------------------------------------------------------------------

class MockLLM {
public:
    explicit MockLLM(MockConfig config);

    /// Perform a single completion (blocking, with simulated latency).
    std::string complete(const std::string& prompt);

    /// Streaming completion — calls on_delta for each chunk, returns full reply.
    std::string complete_stream(const std::string& prompt, StreamCallback on_delta);

    /// Reset scripted index / random seed.
    void reset();

    /// Record of all calls made.
    struct CallRecord {
        std::string prompt;
        std::string response;
    };
    const std::vector<CallRecord>& call_log() const;
    size_t call_count() const;

private:
    MockConfig              m_cfg;
    std::vector<CallRecord> m_log;
    unsigned int            m_seed;
    size_t                  m_script_idx;

    std::string resolve(const std::string& prompt);
    void        simulate_delay() const;
};

/// Global mock slot for process-wide test injection.
void install_mock(MockConfig config);
void uninstall_mock();
bool is_mock_installed();
std::string call_mock(const std::string& prompt);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_MOCK_IMPLEMENTATION

#include <algorithm>
#include <chrono>
#include <sstream>
#include <thread>

namespace llm {
namespace detail_mock {

static unsigned int lcg_rand(unsigned int& state) {
    state = state * 1664525u + 1013904223u;
    return state;
}

static void sleep_ms(double ms) {
    if (ms <= 0.0) return;
    std::this_thread::sleep_for(
        std::chrono::microseconds(static_cast<long long>(ms * 1000.0)));
}

} // namespace detail_mock

MockLLM::MockLLM(MockConfig config)
    : m_cfg(std::move(config))
    , m_seed(42)
    , m_script_idx(0)
{
    if (auto* r = std::get_if<RandomResponses>(&m_cfg.strategy))
        m_seed = r->seed;
    if (auto* s = std::get_if<ScriptedResponses>(&m_cfg.strategy))
        m_script_idx = s->index;
}

std::string MockLLM::resolve(const std::string& prompt) {
    if (auto* s = std::get_if<ScriptedResponses>(&m_cfg.strategy)) {
        if (s->responses.empty()) return "[empty script]";
        std::string r = s->responses[m_script_idx % s->responses.size()];
        ++m_script_idx;
        return r;
    }
    if (auto* p = std::get_if<PatternResponses>(&m_cfg.strategy)) {
        for (const auto& kv : p->patterns)
            if (prompt.find(kv.first) != std::string::npos)
                return kv.second;
        return p->fallback;
    }
    if (auto* r = std::get_if<RandomResponses>(&m_cfg.strategy)) {
        if (r->responses.empty()) return "[empty random]";
        size_t idx = detail_mock::lcg_rand(m_seed) % r->responses.size();
        return r->responses[idx];
    }
    if (auto* e = std::get_if<EchoResponse>(&m_cfg.strategy)) {
        return e->prefix + prompt;
    }
    return "[unknown strategy]";
}

void MockLLM::simulate_delay() const {
    detail_mock::sleep_ms(m_cfg.latency_ms);
}

std::string MockLLM::complete(const std::string& prompt) {
    simulate_delay();
    std::string response = resolve(prompt);
    m_log.push_back({prompt, response});
    return response;
}

std::string MockLLM::complete_stream(const std::string& prompt,
                                      StreamCallback on_delta) {
    std::string response = resolve(prompt);
    m_log.push_back({prompt, response});

    if (!m_cfg.stream_sim || !on_delta) {
        simulate_delay();
        if (on_delta) on_delta(response);
        return response;
    }

    std::istringstream ss(response);
    std::string word;
    std::vector<std::string> words;
    while (ss >> word) words.push_back(word);

    size_t num_chunks = (words.size() + m_cfg.tokens_per_chunk - 1)
                        / std::max<size_t>(1, m_cfg.tokens_per_chunk);
    double chunk_delay = m_cfg.latency_ms > 0.0
        ? m_cfg.latency_ms / std::max<size_t>(1, num_chunks)
        : 0.0;

    size_t i = 0;
    while (i < words.size()) {
        std::string chunk;
        for (size_t j = 0; j < m_cfg.tokens_per_chunk && i < words.size(); ++j, ++i) {
            if (!chunk.empty()) chunk += ' ';
            chunk += words[i];
        }
        detail_mock::sleep_ms(chunk_delay);
        on_delta(chunk);
    }
    return response;
}

void MockLLM::reset() {
    m_script_idx = 0;
    if (auto* r = std::get_if<RandomResponses>(&m_cfg.strategy))
        m_seed = r->seed;
    m_log.clear();
}

const std::vector<MockLLM::CallRecord>& MockLLM::call_log() const { return m_log; }
size_t MockLLM::call_count() const { return m_log.size(); }

namespace {
    bool       g_mock_installed = false;
    MockConfig g_mock_config;
}

void install_mock(MockConfig config) {
    g_mock_config    = std::move(config);
    g_mock_installed = true;
}

void uninstall_mock() { g_mock_installed = false; }
bool is_mock_installed() { return g_mock_installed; }

std::string call_mock(const std::string& prompt) {
    MockLLM m(g_mock_config);
    return m.complete(prompt);
}

} // namespace llm
#endif // LLM_MOCK_IMPLEMENTATION
