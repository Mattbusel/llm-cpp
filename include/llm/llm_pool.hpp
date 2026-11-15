#pragma once

// llm_pool.hpp -- Zero-dependency single-header C++ connection pool for
// concurrent LLM API requests with priority queue and rate limiting.
//
// USAGE:
//   #define LLM_POOL_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_pool.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace llm {

enum class Priority { Low = 0, Normal = 1, High = 2, Critical = 3 };

struct PoolConfig {
    size_t max_concurrent      = 10;
    size_t max_queue_size      = 1000;
    size_t requests_per_minute = 500;
    size_t tokens_per_minute   = 100000;
    double queue_timeout_ms    = 5000.0;
};

struct PoolRequest {
    std::string           id;
    std::function<void()> fn;
    Priority              priority        = Priority::Normal;
    size_t                estimated_tokens = 100;
};

struct PoolResult {
    std::string request_id;
    bool        success    = false;
    double      wait_ms    = 0.0;
    double      execute_ms = 0.0;
    std::string error;
};

class Pool {
public:
    explicit Pool(PoolConfig config = {});
    ~Pool();

    Pool(const Pool&)            = delete;
    Pool& operator=(const Pool&) = delete;

    /// Submit a request asynchronously.
    void submit(PoolRequest request,
                std::function<void(PoolResult)> on_complete = nullptr);

    /// Submit and block until complete.
    PoolResult submit_sync(PoolRequest request);

    /// Block until all in-flight and queued requests complete.
    void drain();

    /// Stop accepting new requests and drain existing ones.
    void shutdown();

    struct Stats {
        size_t in_flight    = 0;
        size_t queued       = 0;
        size_t completed    = 0;
        size_t rejected     = 0;
        size_t rate_limited = 0;
        double avg_wait_ms    = 0.0;
        double avg_execute_ms = 0.0;
    };
    Stats stats() const;

private:
    struct Entry {
        PoolRequest                     req;
        std::function<void(PoolResult)> on_complete;
        std::chrono::steady_clock::time_point enqueued_at;

        bool operator<(const Entry& o) const {
            // lower priority value = lower priority in max-heap
            return static_cast<int>(req.priority) < static_cast<int>(o.req.priority);
        }
    };

    void worker_loop();
    bool rate_limit_ok_locked(size_t tokens);

    PoolConfig m_cfg;

    mutable std::mutex      m_mu;
    std::condition_variable m_cv;
    std::condition_variable m_drain_cv;

    std::priority_queue<Entry>   m_queue;
    std::vector<std::thread>     m_workers;

    std::atomic<bool>   m_shutdown{false};
    std::atomic<size_t> m_in_flight{0};

    // Stats
    size_t m_completed    = 0;
    size_t m_rejected     = 0;
    size_t m_rate_limited = 0;
    double m_total_wait_ms    = 0.0;
    double m_total_execute_ms = 0.0;

    // Token bucket for rate limiting
    size_t m_bucket_requests = 0;
    size_t m_bucket_tokens   = 0;
    std::chrono::steady_clock::time_point m_bucket_reset;
};

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

#ifdef LLM_POOL_IMPLEMENTATION

#include <future>
#include <stdexcept>

namespace llm {

Pool::Pool(PoolConfig config) : m_cfg(std::move(config)) {
    m_bucket_reset = std::chrono::steady_clock::now();
    for (size_t i = 0; i < m_cfg.max_concurrent; ++i)
        m_workers.emplace_back([this]{ worker_loop(); });
}

Pool::~Pool() {
    shutdown();
}

bool Pool::rate_limit_ok_locked(size_t tokens) {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_bucket_reset).count();
    if (elapsed >= 60.0) {
        m_bucket_requests = 0;
        m_bucket_tokens   = 0;
        m_bucket_reset    = now;
    }
    if (m_bucket_requests >= m_cfg.requests_per_minute) return false;
    if (m_bucket_tokens + tokens > m_cfg.tokens_per_minute) return false;
    return true;
}

void Pool::submit(PoolRequest request, std::function<void(PoolResult)> on_complete) {
    std::unique_lock<std::mutex> lock(m_mu);

    if (m_shutdown.load()) {
        ++m_rejected;
        if (on_complete) {
            PoolResult r;
            r.request_id = request.id;
            r.success    = false;
            r.error      = "pool is shut down";
            on_complete(r);
        }
        return;
    }

    if (m_queue.size() >= m_cfg.max_queue_size) {
        ++m_rejected;
        if (on_complete) {
            PoolResult r;
            r.request_id = request.id;
            r.success    = false;
            r.error      = "queue full";
            on_complete(r);
        }
        return;
    }

    if (!rate_limit_ok_locked(request.estimated_tokens)) {
        ++m_rate_limited;
        ++m_rejected;
        if (on_complete) {
            PoolResult r;
            r.request_id = request.id;
            r.success    = false;
            r.error      = "rate limited";
            on_complete(r);
        }
        return;
    }

    m_bucket_requests++;
    m_bucket_tokens += request.estimated_tokens;

    Entry entry;
    entry.req        = std::move(request);
    entry.on_complete = on_complete;
    entry.enqueued_at = std::chrono::steady_clock::now();
    m_queue.push(std::move(entry));
    m_cv.notify_one();
}

PoolResult Pool::submit_sync(PoolRequest request) {
    std::promise<PoolResult> p;
    std::future<PoolResult>  f = p.get_future();
    submit(std::move(request), [&p](PoolResult r){ p.set_value(std::move(r)); });
    return f.get();
}

void Pool::worker_loop() {
    while (true) {
        Entry entry;
        {
            std::unique_lock<std::mutex> lock(m_mu);
            m_cv.wait(lock, [this]{
                return !m_queue.empty() || m_shutdown.load();
            });
            if (m_shutdown.load() && m_queue.empty()) break;
            if (m_queue.empty()) continue;
            entry = m_queue.top();
            m_queue.pop();
            ++m_in_flight;
        }

        auto now = std::chrono::steady_clock::now();
        double wait_ms = std::chrono::duration<double, std::milli>(
            now - entry.enqueued_at).count();

        // Check queue timeout
        if (wait_ms > m_cfg.queue_timeout_ms) {
            PoolResult r;
            r.request_id = entry.req.id;
            r.success    = false;
            r.wait_ms    = wait_ms;
            r.error      = "queue timeout";
            if (entry.on_complete) entry.on_complete(r);
            std::unique_lock<std::mutex> lock(m_mu);
            --m_in_flight;
            ++m_completed;
            m_drain_cv.notify_all();
            continue;
        }

        auto exec_start = std::chrono::steady_clock::now();
        bool success = true;
        std::string error;
        try {
            entry.req.fn();
        } catch (const std::exception& e) {
            success = false; error = e.what();
        } catch (...) {
            success = false; error = "unknown error";
        }
        double exec_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - exec_start).count();

        PoolResult result;
        result.request_id = entry.req.id;
        result.success    = success;
        result.wait_ms    = wait_ms;
        result.execute_ms = exec_ms;
        result.error      = error;

        {
            std::unique_lock<std::mutex> lock(m_mu);
            --m_in_flight;
            ++m_completed;
            m_total_wait_ms    += wait_ms;
            m_total_execute_ms += exec_ms;
            m_drain_cv.notify_all();
        }

        if (entry.on_complete) entry.on_complete(result);
    }
}

void Pool::drain() {
    std::unique_lock<std::mutex> lock(m_mu);
    m_drain_cv.wait(lock, [this]{
        return m_queue.empty() && m_in_flight.load() == 0;
    });
}

void Pool::shutdown() {
    m_shutdown.store(true);
    m_cv.notify_all();
    for (auto& t : m_workers) if (t.joinable()) t.join();
    m_workers.clear();
}

Pool::Stats Pool::stats() const {
    std::lock_guard<std::mutex> lock(m_mu);
    Stats s;
    s.in_flight    = m_in_flight.load();
    s.queued       = m_queue.size();
    s.completed    = m_completed;
    s.rejected     = m_rejected;
    s.rate_limited = m_rate_limited;
    s.avg_wait_ms    = m_completed > 0 ? m_total_wait_ms    / m_completed : 0.0;
    s.avg_execute_ms = m_completed > 0 ? m_total_execute_ms / m_completed : 0.0;
    return s;
}

} // namespace llm

#endif // LLM_POOL_IMPLEMENTATION
