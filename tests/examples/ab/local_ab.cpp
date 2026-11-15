// local_ab.cpp: A/B test without any LLM API calls.
// Uses hardcoded scores to demonstrate Welch t-test and Cohen d.
#define LLM_AB_IMPLEMENTATION
#include "llm_ab.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    // Simulate variant A: scores around 0.7
    llm::VariantResult a;
    a.name = "control";
    a.scores = {0.8, 0.7, 0.6, 0.9, 0.7, 0.8, 0.6, 0.7};
    a.n = a.scores.size();
    a.mean_score = 0.725;
    a.std_dev    = 0.1;

    // Simulate variant B: scores around 0.5
    llm::VariantResult b;
    b.name = "treatment";
    b.scores = {0.5, 0.4, 0.6, 0.5, 0.5, 0.4, 0.6, 0.5};
    b.n = b.scores.size();
    b.mean_score = 0.5;
    b.std_dev    = 0.07;

    // Run stats manually via run_ab_test with no-op variants
    // Demonstrate the statistics output
    std::printf("Simulated A/B test (no API calls)\n");
    std::printf("Control   mean=%.3f  std=%.3f  n=%zu\n", a.mean_score, a.std_dev, a.n);
    std::printf("Treatment mean=%.3f  std=%.3f  n=%zu\n", b.mean_score, b.std_dev, b.n);

    // Compute Welch t manually for demo
    double va = a.std_dev * a.std_dev / (double)a.n;
    double vb = b.std_dev * b.std_dev / (double)b.n;
    double se = std::sqrt(va + vb);
    double t  = (a.mean_score - b.mean_score) / se;
    double pooled = std::sqrt((a.std_dev * a.std_dev + b.std_dev * b.std_dev) / 2.0);
    double d = std::abs(a.mean_score - b.mean_score) / pooled;
    // approximate p-value
    double p = 2.0 / (1.0 + std::exp(0.717 * std::abs(t) + 0.416 * t * t));

    std::printf("t=%.3f  p=%.4f  d=%.3f\n", t, p, d);
    std::printf("Significant (p<0.05)? %s\n", (p < 0.05) ? "YES" : "NO");
    return 0;
}
