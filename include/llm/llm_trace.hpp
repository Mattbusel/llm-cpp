#pragma once

// llm_trace.hpp -- Distributed tracing for LLM pipelines.
// Span tracking, latency breakdown, OTLP-compatible JSON export.
// Zero dependencies. Zero overhead when disabled.
//
// USAGE: #define LLM_TRACE_IMPLEMENTATION in ONE .cpp file.

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstddef>
#include <ctime>

namespace llm {

struct SpanContext {
    std::string trace_id;
    std::string span_id;
    std::string parent_span_id;
};

struct Span {
    SpanContext context;
    std::string name;
    long long   start_ns   = 0;  // nanoseconds since epoch
    long long   end_ns     = 0;
    double      duration_ms = 0.0;
    std::map<std::string, std::string> attributes;
    std::string model;
    size_t      input_tokens  = 0;
    size_t      output_tokens = 0;
    double      cost_usd      = 0.0;
    bool        success       = true;
    std::string error;
    std::vector<Span> children;
};

class Tracer {
public:
    static Tracer& global();

    class ActiveSpan {
    public:
        ActiveSpan(Tracer& tracer, const std::string& name,
                   const std::string& parent_span_id = "");
        ~ActiveSpan();
        ActiveSpan(ActiveSpan&&) noexcept;
        ActiveSpan(const ActiveSpan&) = delete;
        ActiveSpan& operator=(const ActiveSpan&) = delete;

        ActiveSpan child(const std::string& name);
        void set_attribute(const std::string& key, const std::string& value);
        void set_model(const std::string& model);
        void set_tokens(size_t input, size_t output);
        void set_cost(double usd);
        void set_error(const std::string& error);
        void end();
        SpanContext context() const;

    private:
        Tracer*     tracer_   = nullptr;
        Span        span_;
        bool        ended_    = false;
    };

    Tracer();
    ActiveSpan start(const std::string& name);

    const std::vector<Span>& completed_spans() const;
    std::string export_json() const;
    std::string export_text() const;
    void clear();
    void enable();
    void disable();
    bool enabled() const;

private:
    bool              enabled_  = true;
    std::vector<Span> spans_;
    std::string       trace_id_;

    void record(Span s);
    friend class ActiveSpan;
};

#define LLM_TRACE(name) auto _llm_span_##__LINE__ = ::llm::Tracer::global().start(name)

} // namespace llm

// ---------------------------------------------------------------------------
#ifdef LLM_TRACE_IMPLEMENTATION

#include <chrono>
#include <cstdio>
#include <random>
#include <sstream>
#include <iomanip>

namespace llm {
namespace detail_trace {

static std::string gen_id(size_t bytes) {
    static std::mt19937_64 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::string out;
    out.reserve(bytes * 2);
    for (size_t i = 0; i < bytes; ++i) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", (unsigned)(rng() & 0xFF));
        out += buf;
    }
    return out;
}

static long long now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static std::string jesc(const std::string& s) {
    std::string o; o.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else o += (char)c;
    }
    return o;
}

static void span_to_text(const Span& sp, std::string& out, int depth) {
    for (int i = 0; i < depth; ++i) out += "  ";
    out += (depth == 0 ? "" : (sp.children.empty() ? "  " : "+ "));
    out += sp.name;
    char buf[64];
    snprintf(buf, sizeof(buf), " (%.1fms)", sp.duration_ms);
    out += buf;
    if (sp.input_tokens + sp.output_tokens > 0) {
        snprintf(buf, sizeof(buf), " [tok: %zu+%zu]", sp.input_tokens, sp.output_tokens);
        out += buf;
    }
    if (sp.cost_usd > 0.0) {
        snprintf(buf, sizeof(buf), " [$%.4f]", sp.cost_usd);
        out += buf;
    }
    if (!sp.error.empty()) out += " ERROR:" + sp.error;
    out += '\n';
    for (auto& c : sp.children) span_to_text(c, out, depth + 1);
}

} // namespace detail_trace

// ActiveSpan impl
Tracer::ActiveSpan::ActiveSpan(Tracer& tr, const std::string& name,
                                const std::string& parent_span_id)
    : tracer_(&tr) {
    span_.name                    = name;
    span_.context.trace_id        = tr.trace_id_;
    span_.context.span_id         = detail_trace::gen_id(8);
    span_.context.parent_span_id  = parent_span_id;
    span_.start_ns                = detail_trace::now_ns();
}

Tracer::ActiveSpan::ActiveSpan(ActiveSpan&& o) noexcept
    : tracer_(o.tracer_), span_(std::move(o.span_)), ended_(o.ended_) {
    o.tracer_ = nullptr; o.ended_ = true;
}

Tracer::ActiveSpan::~ActiveSpan() { if (!ended_) end(); }

void Tracer::ActiveSpan::end() {
    if (ended_ || !tracer_) return;
    ended_           = true;
    span_.end_ns     = detail_trace::now_ns();
    span_.duration_ms = (span_.end_ns - span_.start_ns) / 1e6;
    tracer_->record(span_);
}

Tracer::ActiveSpan Tracer::ActiveSpan::child(const std::string& name) {
    return ActiveSpan(*tracer_, name, span_.context.span_id);
}

void Tracer::ActiveSpan::set_attribute(const std::string& k, const std::string& v) { span_.attributes[k] = v; }
void Tracer::ActiveSpan::set_model(const std::string& m) { span_.model = m; }
void Tracer::ActiveSpan::set_tokens(size_t in, size_t out) { span_.input_tokens = in; span_.output_tokens = out; }
void Tracer::ActiveSpan::set_cost(double usd) { span_.cost_usd = usd; }
void Tracer::ActiveSpan::set_error(const std::string& e) { span_.error = e; span_.success = false; }
SpanContext Tracer::ActiveSpan::context() const { return span_.context; }

// Tracer impl
Tracer& Tracer::global() { static Tracer t; return t; }

Tracer::Tracer() : enabled_(true), trace_id_(detail_trace::gen_id(16)) {}

Tracer::ActiveSpan Tracer::start(const std::string& name) {
    if (!enabled_) {
        static Tracer dummy; dummy.enabled_ = false;
        return ActiveSpan(dummy, name);
    }
    return ActiveSpan(*this, name);
}

void Tracer::record(Span s) { if (enabled_) spans_.push_back(std::move(s)); }
const std::vector<Span>& Tracer::completed_spans() const { return spans_; }
void Tracer::clear() { spans_.clear(); trace_id_ = detail_trace::gen_id(16); }
void Tracer::enable()  { enabled_ = true; }
void Tracer::disable() { enabled_ = false; }
bool Tracer::enabled() const { return enabled_; }

std::string Tracer::export_text() const {
    std::string out;
    for (auto& sp : spans_) detail_trace::span_to_text(sp, out, 0);
    return out;
}

std::string Tracer::export_json() const {
    // OTLP-compatible simplified JSON
    std::string out = "{\"resourceSpans\":[{\"scopeSpans\":[{\"spans\":[\n";
    bool first = true;
    for (auto& sp : spans_) {
        if (!first) out += ",\n";
        first = false;
        char buf[256];
        out += "  {";
        out += "\"traceId\":\"" + detail_trace::jesc(sp.context.trace_id) + "\",";
        out += "\"spanId\":\"" + detail_trace::jesc(sp.context.span_id) + "\",";
        if (!sp.context.parent_span_id.empty())
            out += "\"parentSpanId\":\"" + detail_trace::jesc(sp.context.parent_span_id) + "\",";
        out += "\"name\":\"" + detail_trace::jesc(sp.name) + "\",";
        snprintf(buf, sizeof(buf), "\"startTimeNs\":%lld,\"endTimeNs\":%lld,\"durationMs\":%.3f",
                 sp.start_ns, sp.end_ns, sp.duration_ms);
        out += buf;
        if (!sp.model.empty()) out += ",\"model\":\"" + detail_trace::jesc(sp.model) + "\"";
        if (sp.input_tokens) { snprintf(buf,sizeof(buf),",\"inputTokens\":%zu",sp.input_tokens); out+=buf; }
        if (sp.output_tokens) { snprintf(buf,sizeof(buf),",\"outputTokens\":%zu",sp.output_tokens); out+=buf; }
        if (sp.cost_usd > 0.0) { snprintf(buf,sizeof(buf),",\"costUsd\":%.6f",sp.cost_usd); out+=buf; }
        out += ",\"success\":" + std::string(sp.success ? "true" : "false");
        if (!sp.error.empty()) out += ",\"error\":\"" + detail_trace::jesc(sp.error) + "\"";
        out += "}";
    }
    out += "\n]}]}]}";
    return out;
}

} // namespace llm
#endif // LLM_TRACE_IMPLEMENTATION
