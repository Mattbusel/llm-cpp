// Regression tests for the four bugs found in
// https://tensorust.com/articles/exponential-backoff-jitter/
// plus Retry-After support. No framework: run the exe, exit code 0 = pass.
#define LLM_RETRY_IMPLEMENTATION
#include "llm_retry.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (cond) { ++g_passed; }                                          \
        else {                                                             \
            ++g_failed;                                                    \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
        }                                                                  \
    } while (0)

static llm::RetryConfig fast_config() {
    llm::RetryConfig cfg;
    cfg.max_attempts  = 3;
    cfg.base_delay_ms = 1.0;
    cfg.max_delay_ms  = 5.0;
    return cfg;
}

// Fails `failures` times with `code`, then returns 42.
static int retried_attempts(int code, int failures) {
    int calls = 0;
    auto r = llm::with_retry<int>([&]() -> int {
        if (++calls <= failures) throw llm::LLMError{code, "transient", true};
        return 42;
    }, fast_config());
    return r.value == 42 ? r.attempts_used : -1;
}

// Fix 1: 529 overloaded and 408 timeout are retried by default.
static void test_retries_529_and_408() {
    std::printf("fix 1: 529 and 408 are retried\n");
    CHECK(retried_attempts(529, 2) == 3);
    CHECK(retried_attempts(408, 2) == 3);
    CHECK(retried_attempts(429, 2) == 3);

    int calls = 0;
    bool threw = false;
    try {
        llm::with_retry<int>([&]() -> int {
            ++calls;
            throw llm::LLMError{400, "invalid_request_error", false};
        }, fast_config());
    } catch (const llm::LLMError& e) { threw = (e.http_code == 400); }
    CHECK(threw);
    CHECK(calls == 1);
}

// Fix 2: an exception from the caller's own code is not retried.
static void test_own_exceptions_not_retried() {
    std::printf("fix 2: caller's own exceptions are not retried\n");
    int calls = 0;
    bool threw = false;
    try {
        llm::with_retry<int>([&]() -> int {
            ++calls;
            throw std::runtime_error("json parse error");
        }, fast_config());
    } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
    CHECK(calls == 1);
}

// Fix 3: with_failover surfaces broken requests instead of hiding them.
static void test_failover_surfaces_client_errors() {
    std::printf("fix 3: failover surfaces 4xx client errors\n");
    for (int code : {400, 401, 403, 404, 413, 422}) {
        int fallback_calls = 0;
        bool threw = false;
        try {
            llm::with_failover<std::string>(
                [&]() -> std::string { throw llm::LLMError{code, "bad request", false}; },
                [&]() -> std::string { ++fallback_calls; return "fallback"; },
                fast_config());
        } catch (const llm::LLMError& e) { threw = (e.http_code == code); }
        CHECK(threw);
        CHECK(fallback_calls == 0);
    }
    // Provider-side failures still fail over.
    for (int code : {408, 429, 503, 529}) {
        auto r = llm::with_failover<std::string>(
            [&]() -> std::string { throw llm::LLMError{code, "provider", true}; },
            []() -> std::string { return "fallback"; },
            fast_config());
        CHECK(r.from_fallback);
        CHECK(r.value == "fallback");
    }
}

// Fix 4: half-open admits a single trial call after the cool-down.
static void test_half_open_single_probe() {
    std::printf("fix 4: half-open lets exactly one probe through\n");
    llm::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 5;
    cfg.timeout_seconds   = 0.2;
    llm::CircuitBreaker br("provider", cfg);

    std::atomic<int> hits{0};
    auto down = [&]() -> int {
        ++hits;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        throw llm::LLMError{503, "down", true};
    };
    for (int i = 0; i < 5; ++i) { try { br.call<int>(down); } catch (...) {} }
    CHECK(br.state() == llm::CircuitState::Open);

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    int before = hits.load();
    std::atomic<int> fast_failed{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 50; ++i) {
        threads.emplace_back([&] {
            try { br.call<int>(down); }
            catch (const std::runtime_error&) { ++fast_failed; }
            catch (...) {}
        });
    }
    for (auto& t : threads) t.join();
    CHECK(hits.load() - before == 1);
    CHECK(fast_failed.load() == 49);
    CHECK(br.state() == llm::CircuitState::Open);   // failed trial reopens

    // Provider back: one successful trial closes the circuit.
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    CHECK(br.call<int>([]() -> int { return 1; }) == 1);
    CHECK(br.state() == llm::CircuitState::Closed);
    CHECK(br.stats().consecutive_failures == 0);
}

// Retry-After: when the server says how long to wait, with_retry waits it.
static void test_retry_after_honored() {
    std::printf("retry-after: server hint is obeyed\n");
    int calls = 0;
    auto r = llm::with_retry<int>([&]() -> int {
        if (++calls == 1) throw llm::LLMError{429, "rate_limit_error", true, 0.3};
        return 7;
    }, fast_config());
    CHECK(r.value == 7);
    CHECK(r.attempts_used == 2);
    CHECK(r.total_elapsed_ms >= 300.0);
    CHECK(r.total_elapsed_ms < 2000.0);
}

int main() {
    test_retries_529_and_408();
    test_own_exceptions_not_retried();
    test_failover_surfaces_client_errors();
    test_half_open_single_probe();
    test_retry_after_honored();
    std::printf("%d checks passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
