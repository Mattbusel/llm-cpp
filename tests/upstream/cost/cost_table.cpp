// Checks the generated model table and the lookup API. Exits non-zero on the
// first failure. Run: cmake -B build && cmake --build build && ctest --test-dir build
#define LLM_COST_IMPLEMENTATION
#include "llm_cost.hpp"

#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,      \
                         __LINE__, #cond);                                   \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

int main() {
    using namespace llm;

    // The twelve constants llm-cost shipped before the table was generated
    // keep their names, ids, prices and context windows.
    CHECK(models::GPT4O.name == "gpt-4o");
    CHECK(models::GPT4O.input_cost_per_1k == 0.0025);
    CHECK(models::GPT4O.output_cost_per_1k == 0.01);
    CHECK(models::GPT4O.context_window == 128000);
    CHECK(models::GPT4O_MINI.input_cost_per_1k == 0.00015);
    CHECK(models::GPT4_TURBO.name == "gpt-4-turbo");
    CHECK(models::GPT6_ASTRA.context_window == 1050000);
    CHECK(models::GPT6_LUNA.input_cost_per_1k == 0.0001);
    CHECK(models::CLAUDE_OPUS.name == "claude-opus-4-5");
    CHECK(models::CLAUDE_OPUS.input_cost_per_1k == 0.005);
    CHECK(models::CLAUDE_SONNET.name == "claude-sonnet-4-5");
    CHECK(models::CLAUDE_HAIKU.output_cost_per_1k == 0.005);
    CHECK(models::CLAUDE_OPUS_5_5.input_cost_per_1k == 0.004);
    CHECK(models::CLAUDE_FABLE_5_1.provider == Provider::Anthropic);

    // The registry: far more than the old twelve, unique (name, provider),
    // every price non-negative, chat models have an output price.
    const auto all = all_models();
    CHECK(all.size() >= 100);
    std::set<std::pair<std::string, int>> seen;
    for (const Model* m : all) {
        CHECK(m != nullptr);
        CHECK(!m->name.empty());
        CHECK(m->input_cost_per_1k >= 0.0);
        CHECK(m->output_cost_per_1k >= 0.0);
        CHECK(seen.insert({m->name, static_cast<int>(m->provider)}).second);
        // Every listed model is found by its own id on its own provider.
        CHECK(find_model(m->name, m->provider) == m);
    }

    // Lookups by id, alias, dated snapshot and case.
    CHECK(find_model("gpt-4o") == &models::GPT4O);
    CHECK(find_model("GPT-4O") == &models::GPT4O);
    CHECK(find_model("claude-sonnet-4-5-20250929") == &models::CLAUDE_SONNET);
    CHECK(find_model("gpt-4-turbo-2024-04-09") == &models::GPT4_TURBO);
    const Model* ds = find_model("deepseek-chat");
    CHECK(ds && ds->provider == Provider::DeepSeek);
    const Model* ml = find_model("mistral-large-latest");
    CHECK(ml && ml->provider == Provider::Mistral);
    CHECK(find_model("gemini-2.5-flash") && find_model("gemini-2.5-flash")->provider == Provider::Google);
    CHECK(find_model("no-such-model") == nullptr);
    CHECK(find_model("") == nullptr);

    // Retired models are not in all_models() but can still be priced.
    const Model* old = find_model("gpt-3.5-turbo");
    CHECK(old != nullptr);
    for (const Model* m : all) CHECK(m != old);

    // The same open model on several hosts: the provider picks the row.
    const Model* tg = find_model("openai/gpt-oss-120b", Provider::Together);
    const Model* gq = find_model("openai/gpt-oss-120b", Provider::Groq);
    CHECK(tg && tg->provider == Provider::Together);
    CHECK(gq && gq->provider == Provider::Groq);
    CHECK(find_model("openai/gpt-oss-120b", Provider::Anthropic) == nullptr);

    // compare_costs: sorted, no embedding models, one token count.
    const std::string prompt = "Summarise the attached incident report in five bullet points.";
    const auto rows = compare_costs(prompt);
    CHECK(rows.size() > 90 && rows.size() <= all.size());
    for (size_t i = 1; i < rows.size(); ++i) {
        CHECK(rows[i - 1].input_cost_usd <= rows[i].input_cost_usd);
        CHECK(rows[i].tokens == rows[0].tokens);
    }
    for (const auto& r : rows) CHECK(r.model_name.rfind("text-embedding", 0) != 0);

    // compare_costs over a chosen list keeps only those models.
    const auto two = compare_costs(prompt, {models::CLAUDE_OPUS, models::GPT4O_MINI});
    CHECK(two.size() == 2);
    CHECK(two.size() == 2 && two[0].model_name == "gpt-4o-mini");
    CHECK(compare_costs(prompt, {}).empty());

    // context_window 0 means unknown: never reported as exceeded.
    Model unknown{"my-model", Provider::OpenAI, 0.001, 0.002, 0};
    CHECK(!count(std::string(100000, 'a'), unknown).exceeds_context);
    Model tiny{"tiny", Provider::OpenAI, 0.001, 0.002, 10};
    CHECK(count(std::string(1000, 'a'), tiny).exceeds_context);

    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("cost_table: all checks passed (%zu listed models, %zu in compare_costs)\n",
                all.size(), rows.size());
    return EXIT_SUCCESS;
}
