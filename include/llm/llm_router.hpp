#pragma once
#define NOMINMAX

// llm_router.hpp -- Zero-dependency single-header C++ prompt router.
// Routes prompts to different models based on complexity scoring and strategy.
// Supports CostOptimized, LatencyOptimized, QualityOptimized, Balanced, Budget.
// Optional EMA learning from outcomes.
//
// USAGE:
//   #define LLM_ROUTER_IMPLEMENTATION  (in exactly one .cpp)
//   #include "llm_router.hpp"
//
// No external dependencies required (routing only; completions via other headers).

#include <string>
#include <vector>
#include <map>

namespace llm {

enum class RoutingStrategy {
    CostOptimized,      // prefer cheapest model able to handle complexity
    LatencyOptimized,   // prefer fastest model
    QualityOptimized,   // prefer highest quality model
    Balanced,           // balance quality/cost/latency score
    Budget,             // hard cap: never route above max_cost_tier
};

struct ModelProfile {
    std::string name;
    double      cost_per_1k   = 1.0;  // relative cost (arbitrary units)
    double      latency_score = 1.0;  // lower = faster (relative)
    double      quality_score = 1.0;  // higher = better
    int         max_complexity = 100; // max complexity score this model handles well
};

struct RouterConfig {
    RoutingStrategy        strategy    = RoutingStrategy::Balanced;
    std::vector<ModelProfile> models;  // ordered from cheapest/fastest to best
    int                    max_cost_tier = 100; // for Budget strategy
    bool                   learn_from_outcomes = false; // EMA adaptation
    double                 ema_alpha   = 0.1;  // EMA learning rate
};

struct RouteDecision {
    std::string model_name;
    int         complexity_score; // 0-100
    std::string reason;
};

/// Score a prompt's complexity (0-100).
int score_complexity(const std::string& prompt);

class Router {
public:
    explicit Router(RouterConfig config);

    /// Route a prompt to the best model given strategy.
    RouteDecision route(const std::string& prompt) const;

    /// Record outcome quality (0.0-1.0) for the given model to adapt routing.
    void record_outcome(const std::string& model_name, double quality);

    /// Effective quality score for a model (after learning).
    double effective_quality(const std::string& model_name) const;

private:
    RouterConfig                    m_cfg;
    std::map<std::string, double>   m_learned_quality; // EMA-adjusted quality

    double model_score(const ModelProfile& m, int complexity,
                       RoutingStrategy strategy) const;
};

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_ROUTER_IMPLEMENTATION

#include <algorithm>
#include <cctype>
#include <sstream>

namespace llm {

int score_complexity(const std::string& prompt) {
    if (prompt.empty()) return 0;

    int score = 0;

    // Length factor (up to 30 points)
    size_t len = prompt.size();
    if (len > 2000)      score += 30;
    else if (len > 1000) score += 20;
    else if (len > 500)  score += 15;
    else if (len > 200)  score += 8;
    else                 score += 2;

    // Question count (up to 15 points)
    int questions = 0;
    for (char c : prompt) if (c == '?') ++questions;
    score += std::min(15, questions * 5);

    // Technical keyword density (up to 25 points)
    static const char* tech_words[] = {
        "algorithm", "function", "code", "implement", "optimize",
        "architecture", "analyze", "compare", "explain", "calculate",
        "derive", "prove", "translate", "refactor", "debug",
        nullptr
    };
    std::string lower = prompt;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    int kw_hits = 0;
    for (int i = 0; tech_words[i]; ++i)
        if (lower.find(tech_words[i]) != std::string::npos) ++kw_hits;
    score += std::min(25, kw_hits * 4);

    // Multi-step indicators (up to 15 points)
    static const char* step_words[] = {
        "first", "second", "third", "then", "finally", "step", "next", nullptr
    };
    int step_hits = 0;
    for (int i = 0; step_words[i]; ++i)
        if (lower.find(step_words[i]) != std::string::npos) ++step_hits;
    score += std::min(15, step_hits * 3);

    // Code block presence (15 points)
    if (prompt.find("```") != std::string::npos) score += 15;

    return std::min(100, score);
}

Router::Router(RouterConfig config) : m_cfg(std::move(config)) {
    for (const auto& m : m_cfg.models)
        m_learned_quality[m.name] = m.quality_score;
}

double Router::effective_quality(const std::string& model_name) const {
    auto it = m_learned_quality.find(model_name);
    if (it != m_learned_quality.end()) return it->second;
    // fallback: find in profiles
    for (const auto& m : m_cfg.models)
        if (m.name == model_name) return m.quality_score;
    return 0.0;
}

double Router::model_score(const ModelProfile& m, int complexity,
                            RoutingStrategy strategy) const {
    // Penalise if complexity exceeds what the model handles well
    double complexity_fit = (complexity <= m.max_complexity) ? 1.0
        : 1.0 - 0.5 * static_cast<double>(complexity - m.max_complexity) / 100.0;

    double eq = effective_quality(m.name);

    switch (strategy) {
        case RoutingStrategy::CostOptimized:
            // High score = low cost * fits complexity
            return complexity_fit * (1.0 / std::max(0.001, m.cost_per_1k));

        case RoutingStrategy::LatencyOptimized:
            return complexity_fit * (1.0 / std::max(0.001, m.latency_score));

        case RoutingStrategy::QualityOptimized:
            return complexity_fit * eq;

        case RoutingStrategy::Balanced:
            return complexity_fit * (eq / std::max(0.001, m.cost_per_1k * m.latency_score));

        case RoutingStrategy::Budget:
            // Filter by cost tier, then pick best quality
            if (m.cost_per_1k > static_cast<double>(m_cfg.max_cost_tier)) return -1.0;
            return complexity_fit * eq;
    }
    return 0.0;
}

RouteDecision Router::route(const std::string& prompt) const {
    int complexity = score_complexity(prompt);

    if (m_cfg.models.empty())
        return {"gpt-4o-mini", complexity, "no models configured"};

    const ModelProfile* best = nullptr;
    double best_score = -1e18;

    for (const auto& m : m_cfg.models) {
        double s = model_score(m, complexity, m_cfg.strategy);
        if (s > best_score) { best_score = s; best = &m; }
    }

    if (!best) best = &m_cfg.models.front();

    std::string reason;
    switch (m_cfg.strategy) {
        case RoutingStrategy::CostOptimized:    reason = "cost-optimized"; break;
        case RoutingStrategy::LatencyOptimized: reason = "latency-optimized"; break;
        case RoutingStrategy::QualityOptimized: reason = "quality-optimized"; break;
        case RoutingStrategy::Balanced:         reason = "balanced"; break;
        case RoutingStrategy::Budget:           reason = "budget-capped"; break;
    }
    reason += ", complexity=" + std::to_string(complexity);

    return {best->name, complexity, reason};
}

void Router::record_outcome(const std::string& model_name, double quality) {
    if (!m_cfg.learn_from_outcomes) return;
    auto it = m_learned_quality.find(model_name);
    if (it == m_learned_quality.end()) {
        m_learned_quality[model_name] = quality;
    } else {
        it->second = (1.0 - m_cfg.ema_alpha) * it->second + m_cfg.ema_alpha * quality;
    }
}

} // namespace llm
#endif // LLM_ROUTER_IMPLEMENTATION
