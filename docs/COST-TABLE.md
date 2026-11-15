# The llm-cost model table

`include/llm/llm_cost.hpp` prices prompts against a built-in table of models.
Since llm-cpp 1.1.0 that table is generated from
[llm-prices](https://github.com/Mattbusel/llm-prices), which keeps the standard
per-token price of every model in one `prices.json`, checked against each
provider's pricing page (OpenAI, Anthropic, Google, DeepSeek, Mistral, xAI,
Cohere, Groq, Together, Fireworks).

## What is generated

`tools/gen_cost_table.py` rewrites two marked regions of the header and leaves
the rest alone:

| Region | Contents |
|---|---|
| `BEGIN GENERATED PROVIDERS` | `enum class Provider` (OpenAI and Anthropic keep values 0 and 1) |
| `BEGIN GENERATED MODELS` | one `inline const llm::Model` per model, the listed and retired lists behind `all_models()` and `find_model()`, and the alias table |

Prices are USD per 1K tokens at the standard rate: no batch, cache or
long-context tiers. Models the providers no longer list are kept as constants
and found by `find_model()`, but are left out of `all_models()` and
`compare_costs(text)`. Embedding models have an output price of 0 and are left
out of `compare_costs(text)`.

Constant names are the model id in upper case with every run of other
characters turned into `_`. Ids on Groq, Together and Fireworks get the host as
a prefix (`TOGETHER_OPENAI_GPT_OSS_120B`), because the same open model is
served by several hosts. The twelve constants llm-cost had before the table was
generated (`GPT4O`, `GPT4O_MINI`, `GPT4_TURBO`, `GPT6_ASTRA`, `GPT6_SOL`,
`GPT6_LUNA`, `CLAUDE_FABLE_5_1`, `CLAUDE_OPUS_5_5`, `CLAUDE_SONNET_5`,
`CLAUDE_OPUS`, `CLAUDE_SONNET`, `CLAUDE_HAIKU`) keep their names, ids and
context windows. llm-prices has no context windows, so every other model has
`context_window == 0` ("not known"), which `count()` never reports as exceeded.

## Lookups

```cpp
const llm::Model* m = llm::find_model("deepseek-chat");          // alias
const llm::Model* s = llm::find_model("claude-sonnet-4-5-20250929");
const llm::Model* t = llm::find_model("openai/gpt-oss-120b", llm::Provider::Together);
for (const llm::Model* each : llm::all_models()) { /* 131 today */ }
auto rows = llm::compare_costs(prompt, {*m, *s});                // your shortlist
```

Matching is exact on the id or an alias, ignoring ASCII case. Unknown ids return
`nullptr`.

## Updating

```sh
python tools/gen_cost_table.py --check     # exit 1 and print the diff if stale
python tools/gen_cost_table.py             # rewrite include/llm/llm_cost.hpp
```

By default it reads `prices.json` from llm-prices' `main` branch; pass
`--prices FILE_OR_URL` to pin another copy, for example a release tag:
`https://raw.githubusercontent.com/Mattbusel/llm-prices/v1.3.0/prices.json`.

The header is vendored byte for byte from
[Mattbusel/llm-cost](https://github.com/Mattbusel/llm-cost), so an update is:

1. `python tools/gen_cost_table.py --header ../llm-cost/include/llm_cost.hpp`,
   build and run llm-cost's `tests/cost_table.cpp`, commit there.
2. `python tools/sync_headers.py` here, which copies the header and records the
   new commit in `include/llm/SOURCES.md`.
3. Rebuild, run `ctest`, and copy the new output of `offline_cost` to
   `examples/offline/output/cost.txt` if a price in the example changed.

The [Cost table workflow](../.github/workflows/cost-table.yml) runs `--check`
every Monday, so a price change in llm-prices shows up as a failed run.
