#define LLM_COST_IMPLEMENTATION
#include "llm_cost.hpp"

#include <iostream>

static const std::string LONG_PROMPT = R"(
You are an expert software engineer with deep knowledge of distributed systems,
database internals, and high-performance computing. I need you to help me design
a system that can handle 10 million requests per second with sub-millisecond
latency at the 99th percentile. The system needs to be horizontally scalable,
fault-tolerant, and support strong consistency guarantees.

Please provide a detailed architecture overview covering:
1. Load balancing strategy (L4 vs L7, consistent hashing, etc.)
2. Data partitioning and sharding approach
3. Consensus mechanism (Raft, Paxos, or simpler quorum-based)
4. Caching layers (L1/L2/L3 cache hierarchy, cache invalidation)
5. Network topology and data-center placement
6. Failure detection and recovery procedures
7. Monitoring, alerting, and observability stack
8. Capacity planning formulas and back-of-envelope calculations

Be specific about trade-offs. I prefer concrete recommendations over general
principles, and I want exact technology choices with justification.
)";

int main() {
    using namespace llm;

    const Model& model = models::GPT4O_MINI;

    TokenCount tc = count(LONG_PROMPT, model);

    std::cout << "=== Token Count Report ===\n\n";
    std::cout << "Model:           " << tc.model_name       << "\n";
    std::cout << "Characters:      " << tc.characters        << "\n";
    std::cout << "Estimated tokens:" << tc.tokens            << "\n";
    std::cout << "Input cost:      " << format_cost(tc.estimated_cost_usd) << "\n";
    std::cout << "Exceeds context: " << (tc.exceeds_context ? "YES" : "no") << "\n";

    // Also show output cost estimate (assume 2x reply length as rough guess)
    const double output_tokens_est = tc.tokens * 2.0;
    const double output_cost = (output_tokens_est / 1000.0) * model.output_cost_per_1k;
    std::cout << "Est. output cost (2x reply): " << format_cost(output_cost) << "\n";
    std::cout << "Est. total cost:             "
              << format_cost(tc.estimated_cost_usd + output_cost) << "\n";

    return 0;
}
