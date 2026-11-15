#pragma once

// llm_cost.hpp: Zero-dependency single-header C++ token counter and cost estimator.
// Prices 130+ models from OpenAI, Anthropic, Google, DeepSeek, Mistral, xAI,
// Cohere, Groq, Together and Fireworks. No network calls. No external deps.
//
// USAGE:
//   In exactly ONE .cpp file:
//     #define LLM_COST_IMPLEMENTATION
//     #include "llm_cost.hpp"
//
//   In all other files:
//     #include "llm_cost.hpp"
//
// Token counting uses a cl100k_base approximation (~±5% vs tiktoken). See README.

#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iterator>

namespace llm {

// ---------------------------------------------------------------------------
// Public types
// ---------------------------------------------------------------------------

// BEGIN GENERATED PROVIDERS (tools/gen_cost_table.py in Mattbusel/llm-cpp)
enum class Provider {
    OpenAI,
    Anthropic,
    Google,
    DeepSeek,
    Mistral,
    XAI,
    Cohere,
    Groq,
    Together,
    Fireworks,
};
// END GENERATED PROVIDERS

/// Pricing and metadata for one model.
struct Model {
    std::string name;
    Provider    provider;
    double      input_cost_per_1k;   ///< USD per 1 000 input tokens
    double      output_cost_per_1k;  ///< USD per 1 000 output tokens
    size_t      context_window;      ///< Maximum context in tokens, 0 = not known
};

// ---------------------------------------------------------------------------
// Built-in model registry
//
// Prices are USD per 1K tokens at each provider's standard (non-batch,
// non-cached, short-context) rate. The table below is generated from
// llm-prices (https://github.com/Mattbusel/llm-prices) by
// tools/gen_cost_table.py in https://github.com/Mattbusel/llm-cpp; do not edit
// it by hand. Prices change; regenerate, or define your own llm::Model.
// ---------------------------------------------------------------------------
// BEGIN GENERATED MODELS
// Source: llm-prices prices.json, generated 2026-11-15,
// sha256 4539a11ba1c22d65. 131 listed models, 15 retired.
// context_window is 0 (not known) except for the models llm-cost shipped
// before the table was generated.
namespace models {
    // Anthropic
    inline const Model CLAUDE_FABLE_5_2         = {"claude-fable-5-2", Provider::Anthropic, 0.008, 0.04, 0};
    inline const Model CLAUDE_FABLE_5_1         = {"claude-fable-5-1", Provider::Anthropic, 0.01, 0.05, 1000000};
    inline const Model CLAUDE_FABLE_5           = {"claude-fable-5", Provider::Anthropic, 0.01, 0.05, 0};
    inline const Model CLAUDE_OPUS_5_5          = {"claude-opus-5-5", Provider::Anthropic, 0.004, 0.02, 1000000};
    inline const Model CLAUDE_OPUS_5            = {"claude-opus-5", Provider::Anthropic, 0.005, 0.025, 0};
    inline const Model CLAUDE_OPUS_4_8          = {"claude-opus-4-8", Provider::Anthropic, 0.005, 0.025, 0};
    inline const Model CLAUDE_OPUS_4_7          = {"claude-opus-4-7", Provider::Anthropic, 0.005, 0.025, 0};
    inline const Model CLAUDE_OPUS_4_6          = {"claude-opus-4-6", Provider::Anthropic, 0.005, 0.025, 0};
    inline const Model CLAUDE_OPUS              = {"claude-opus-4-5", Provider::Anthropic, 0.005, 0.025, 200000};
    inline const Model CLAUDE_SONNET_5_5        = {"claude-sonnet-5-5", Provider::Anthropic, 0.002, 0.01, 0};
    inline const Model CLAUDE_SONNET_5          = {"claude-sonnet-5", Provider::Anthropic, 0.002, 0.01, 1000000};
    inline const Model CLAUDE_SONNET_4_6        = {"claude-sonnet-4-6", Provider::Anthropic, 0.003, 0.015, 0};
    inline const Model CLAUDE_SONNET            = {"claude-sonnet-4-5", Provider::Anthropic, 0.003, 0.015, 200000};
    inline const Model CLAUDE_HAIKU_5_5         = {"claude-haiku-5-5", Provider::Anthropic, 0.0001, 0.0005, 0};
    inline const Model CLAUDE_HAIKU             = {"claude-haiku-4-5", Provider::Anthropic, 0.001, 0.005, 200000};
    // OpenAI
    inline const Model GPT6_ASTRA               = {"gpt-6-astra", Provider::OpenAI, 0.01, 0.05, 1050000};
    inline const Model GPT_6_1_SOL              = {"gpt-6.1-sol", Provider::OpenAI, 0.002, 0.01, 0};
    inline const Model GPT_6_1_TERRA            = {"gpt-6.1-terra", Provider::OpenAI, 0.001, 0.005, 0};
    inline const Model GPT6_LUNA                = {"gpt-6-luna", Provider::OpenAI, 0.0001, 0.0005, 1050000};
    inline const Model GPT6_SOL                 = {"gpt-6-sol", Provider::OpenAI, 0.002, 0.01, 1050000};
    inline const Model GPT_5_6_SOL              = {"gpt-5.6-sol", Provider::OpenAI, 0.004, 0.02, 0};
    inline const Model GPT_5_6_TERRA            = {"gpt-5.6-terra", Provider::OpenAI, 0.002, 0.012, 0};
    inline const Model GPT_5_6_LUNA             = {"gpt-5.6-luna", Provider::OpenAI, 0.0002, 0.0012, 0};
    inline const Model GPT_5_5                  = {"gpt-5.5", Provider::OpenAI, 0.005, 0.03, 0};
    inline const Model GPT_5_5_PRO              = {"gpt-5.5-pro", Provider::OpenAI, 0.03, 0.18, 0};
    inline const Model GPT_5_4                  = {"gpt-5.4", Provider::OpenAI, 0.0025, 0.015, 0};
    inline const Model GPT_5_4_PRO              = {"gpt-5.4-pro", Provider::OpenAI, 0.03, 0.18, 0};
    inline const Model GPT_5_4_MINI             = {"gpt-5.4-mini", Provider::OpenAI, 0.00075, 0.0045, 0};
    inline const Model GPT_5_4_NANO             = {"gpt-5.4-nano", Provider::OpenAI, 0.0002, 0.00125, 0};
    inline const Model GPT_5_2                  = {"gpt-5.2", Provider::OpenAI, 0.00175, 0.014, 0};
    inline const Model GPT_5_2_PRO              = {"gpt-5.2-pro", Provider::OpenAI, 0.021, 0.168, 0};
    inline const Model GPT_5_1                  = {"gpt-5.1", Provider::OpenAI, 0.00125, 0.01, 0};
    inline const Model GPT_5                    = {"gpt-5", Provider::OpenAI, 0.00125, 0.01, 0};
    inline const Model GPT_5_MINI               = {"gpt-5-mini", Provider::OpenAI, 0.00025, 0.002, 0};
    inline const Model GPT_5_NANO               = {"gpt-5-nano", Provider::OpenAI, 0.00005, 0.0004, 0};
    inline const Model GPT_5_PRO                = {"gpt-5-pro", Provider::OpenAI, 0.015, 0.12, 0};
    inline const Model GPT_4_1                  = {"gpt-4.1", Provider::OpenAI, 0.002, 0.008, 0};
    inline const Model GPT_4_1_MINI             = {"gpt-4.1-mini", Provider::OpenAI, 0.0004, 0.0016, 0};
    inline const Model GPT4O                    = {"gpt-4o", Provider::OpenAI, 0.0025, 0.01, 128000};
    inline const Model GPT4O_MINI               = {"gpt-4o-mini", Provider::OpenAI, 0.00015, 0.0006, 128000};
    inline const Model O3_PRO                   = {"o3-pro", Provider::OpenAI, 0.02, 0.08, 0};
    inline const Model O3                       = {"o3", Provider::OpenAI, 0.002, 0.008, 0};
    inline const Model GPT_6_CYBER              = {"gpt-6-cyber", Provider::OpenAI, 0.02, 0.1, 0};
    inline const Model GPT_5_6_CYBER            = {"gpt-5.6-cyber", Provider::OpenAI, 0.0125, 0.075, 0};
    inline const Model GPT_5_5_CYBER_PREVIEW    = {"gpt-5.5-cyber-preview", Provider::OpenAI, 0.0125, 0.075, 0};
    inline const Model GPT_6_ROSALIND           = {"gpt-6-rosalind", Provider::OpenAI, 0.015, 0.075, 0};
    inline const Model GPT_ROSALIND_RESEARCH    = {"gpt-rosalind-research", Provider::OpenAI, 0.005, 0.025, 0};
    inline const Model GPT_ROSALIND_DISCOVERY   = {"gpt-rosalind-discovery", Provider::OpenAI, 0.005, 0.025, 0};
    inline const Model GPT_6_ASTRA_LAW          = {"gpt-6-astra-law", Provider::OpenAI, 0.01, 0.05, 0};
    inline const Model CHAT_LATEST              = {"chat-latest", Provider::OpenAI, 0.005, 0.03, 0};
    inline const Model GPT_5_3_CODEX            = {"gpt-5.3-codex", Provider::OpenAI, 0.00175, 0.014, 0};
    inline const Model GPT_5_SEARCH_API         = {"gpt-5-search-api", Provider::OpenAI, 0.00125, 0.01, 0};
    inline const Model TEXT_EMBEDDING_3_SMALL   = {"text-embedding-3-small", Provider::OpenAI, 0.00002, 0.0, 0};
    inline const Model TEXT_EMBEDDING_3_LARGE   = {"text-embedding-3-large", Provider::OpenAI, 0.00013, 0.0, 0};
    inline const Model TEXT_EMBEDDING_ADA_002   = {"text-embedding-ada-002", Provider::OpenAI, 0.0001, 0.0, 0};
    // Google
    inline const Model GEMINI_4_CARBON          = {"gemini-4-carbon", Provider::Google, 0.002, 0.01, 0};
    inline const Model GEMINI_4_ARGON           = {"gemini-4-argon", Provider::Google, 0.002, 0.01, 0};
    inline const Model GEMINI_4_HYDROGEN        = {"gemini-4-hydrogen", Provider::Google, 0.0015, 0.0075, 0};
    inline const Model GEMINI_3_8_FLASH         = {"gemini-3.8-flash", Provider::Google, 0.00075, 0.00375, 0};
    inline const Model GEMINI_3_6_FLASH         = {"gemini-3.6-flash", Provider::Google, 0.00075, 0.00375, 0};
    inline const Model GEMINI_3_5_FLASH_LITE    = {"gemini-3.5-flash-lite", Provider::Google, 0.0003, 0.0025, 0};
    inline const Model GEMINI_3_1_FLASH_LITE    = {"gemini-3.1-flash-lite", Provider::Google, 0.00025, 0.0015, 0};
    inline const Model GEMINI_3_1_PRO_PREVIEW   = {"gemini-3.1-pro-preview", Provider::Google, 0.002, 0.012, 0};
    inline const Model GEMINI_3_FLASH_PREVIEW   = {"gemini-3-flash-preview", Provider::Google, 0.0005, 0.003, 0};
    inline const Model GEMINI_2_5_PRO           = {"gemini-2.5-pro", Provider::Google, 0.00125, 0.01, 0};
    inline const Model GEMINI_2_5_FLASH         = {"gemini-2.5-flash", Provider::Google, 0.0003, 0.0025, 0};
    inline const Model GEMINI_2_5_FLASH_LITE    = {"gemini-2.5-flash-lite", Provider::Google, 0.0001, 0.0004, 0};
    // DeepSeek
    inline const Model DEEPSEEK_FLASH           = {"deepseek-flash", Provider::DeepSeek, 0.0003, 0.0012, 0};
    inline const Model DEEPSEEK_V4_PRO          = {"deepseek-v4-pro", Provider::DeepSeek, 0.00132, 0.00396, 0};
    // Mistral
    inline const Model MISTRAL_LARGE_4          = {"mistral-large-4", Provider::Mistral, 0.0007, 0.0021, 0};
    inline const Model MISTRAL_SMALL_2603       = {"mistral-small-2603", Provider::Mistral, 0.00015, 0.0006, 0};
    inline const Model MINISTRAL_14B_2512       = {"ministral-14b-2512", Provider::Mistral, 0.0002, 0.0002, 0};
    inline const Model MINISTRAL_8B_2512        = {"ministral-8b-2512", Provider::Mistral, 0.00015, 0.00015, 0};
    inline const Model MINISTRAL_3B_2512        = {"ministral-3b-2512", Provider::Mistral, 0.0001, 0.0001, 0};
    inline const Model CODESTRAL_2508           = {"codestral-2508", Provider::Mistral, 0.0003, 0.0009, 0};
    inline const Model ZAI_GLM_5_3              = {"zai-glm-5-3", Provider::Mistral, 0.0014, 0.0044, 0};
    inline const Model ZAI_GLM_5_3_FLASH        = {"zai-glm-5-3-flash", Provider::Mistral, 0.00015, 0.0005, 0};
    inline const Model DEEPSEEK_V4_1_FLASH_1017 = {"deepseek-v4-1-flash-1017", Provider::Mistral, 0.0003, 0.0012, 0};
    inline const Model MISTRAL_EMBED_2312       = {"mistral-embed-2312", Provider::Mistral, 0.0001, 0.0, 0};
    inline const Model CODESTRAL_EMBED_2505     = {"codestral-embed-2505", Provider::Mistral, 0.00015, 0.0, 0};
    // XAI
    inline const Model GROK_4_7                 = {"grok-4.7", Provider::XAI, 0.002, 0.006, 0};
    inline const Model GROK_4_6                 = {"grok-4.6", Provider::XAI, 0.002, 0.006, 0};
    inline const Model GROK_4_5                 = {"grok-4.5", Provider::XAI, 0.002, 0.006, 0};
    inline const Model GROK_4_3                 = {"grok-4.3", Provider::XAI, 0.00125, 0.0025, 0};
    inline const Model GROK_4_20_0309_REASONING = {"grok-4.20-0309-reasoning", Provider::XAI, 0.00125, 0.0025, 0};
    inline const Model GROK_4_20_0309_NON_REASONING = {"grok-4.20-0309-non-reasoning", Provider::XAI, 0.00125, 0.0025, 0};
    inline const Model GROK_4_20_MULTI_AGENT_0309 = {"grok-4.20-multi-agent-0309", Provider::XAI, 0.00125, 0.0025, 0};
    inline const Model GROK_BUILD_0_1           = {"grok-build-0.1", Provider::XAI, 0.001, 0.002, 0};
    // Cohere
    inline const Model COMMAND_R_PLUS_08_2024   = {"command-r-plus-08-2024", Provider::Cohere, 0.0025, 0.01, 0};
    // Groq
    inline const Model GROQ_OPENAI_GPT_OSS_120B = {"openai/gpt-oss-120b", Provider::Groq, 0.00015, 0.0006, 0};
    inline const Model GROQ_OPENAI_GPT_OSS_20B  = {"openai/gpt-oss-20b", Provider::Groq, 0.000075, 0.0003, 0};
    inline const Model GROQ_OPENAI_GPT_OSS_SAFEGUARD_20B = {"openai/gpt-oss-safeguard-20b", Provider::Groq, 0.000075, 0.0003, 0};
    inline const Model GROQ_QWEN_QWEN3_8_27B    = {"qwen/qwen3.8-27b", Provider::Groq, 0.0008, 0.004, 0};
    inline const Model GROQ_META_LLAMA_LLAMA_PROMPT_GUARD_2_22M = {"meta-llama/llama-prompt-guard-2-22m", Provider::Groq, 0.00003, 0.00003, 0};
    inline const Model GROQ_META_LLAMA_LLAMA_PROMPT_GUARD_2_86M = {"meta-llama/llama-prompt-guard-2-86m", Provider::Groq, 0.00004, 0.00004, 0};
    // Together
    inline const Model TOGETHER_DEEPSEEK_AI_DEEPSEEK_V4_PRO_0813 = {"deepseek-ai/DeepSeek-V4-Pro-0813", Provider::Together, 0.00132, 0.00396, 0};
    inline const Model TOGETHER_DEEPSEEK_AI_DEEPSEEK_V4_1_FLASH = {"deepseek-ai/DeepSeek-V4.1-Flash", Provider::Together, 0.0003, 0.0012, 0};
    inline const Model TOGETHER_DEEPSEEK_AI_DEEPSEEK_V4_1_FLASH_1017 = {"deepseek-ai/DeepSeek-V4.1-Flash-1017", Provider::Together, 0.0003, 0.0012, 0};
    inline const Model TOGETHER_ELORIAN_LABS_GLIMPSE_1_0 = {"elorian-labs/glimpse-1.0", Provider::Together, 0.0006, 0.003, 0};
    inline const Model TOGETHER_META_MODELS_MUSE_GLIMMER_30B = {"meta-models/Muse-Glimmer-30B", Provider::Together, 0.00035, 0.0015, 0};
    inline const Model TOGETHER_META_MODELS_MUSE_SPARK_OPEN_816B_A42B = {"meta-models/Muse-Spark-Open-816B-A42B", Provider::Together, 0.00125, 0.00425, 0};
    inline const Model TOGETHER_MINIMAXAI_MINIMAX_M3_1_FLASH = {"MiniMaxAI/MiniMax-M3.1-Flash", Provider::Together, 0.00015, 0.0006, 0};
    inline const Model TOGETHER_MOONSHOTAI_KIMI_K3 = {"moonshotai/Kimi-K3", Provider::Together, 0.003, 0.015, 0};
    inline const Model TOGETHER_MOONSHOTAI_KIMI_K3_1_PRO = {"moonshotai/Kimi-K3.1-Pro", Provider::Together, 0.002, 0.01, 0};
    inline const Model TOGETHER_NVIDIA_NVIDIA_NEMOTRON_3_5_SUPER_VL = {"nvidia/NVIDIA-Nemotron-3.5-Super-VL", Provider::Together, 0.00015, 0.0005, 0};
    inline const Model TOGETHER_OPENAI_GPT_OSS_120B = {"openai/gpt-oss-120b", Provider::Together, 0.00015, 0.0006, 0};
    inline const Model TOGETHER_QWEN_QWEN3_5_9B = {"Qwen/Qwen3.5-9B", Provider::Together, 0.00017, 0.00025, 0};
    inline const Model TOGETHER_QWEN_QWEN3_6_PLUS = {"Qwen/Qwen3.6-Plus", Provider::Together, 0.0005, 0.003, 0};
    inline const Model TOGETHER_QWEN_QWEN3_7_MAX = {"Qwen/Qwen3.7-Max", Provider::Together, 0.0025, 0.0075, 0};
    inline const Model TOGETHER_QWEN_QWEN3_7_PLUS = {"Qwen/Qwen3.7-Plus", Provider::Together, 0.00032, 0.00128, 0};
    inline const Model TOGETHER_QWEN_QWEN3_8_FLASH = {"Qwen/Qwen3.8-Flash", Provider::Together, 0.00015, 0.00047, 0};
    inline const Model TOGETHER_REFLECTION_BEAM_1_0_501B_A23B = {"reflection/Beam-1.0-501B-A23B", Provider::Together, 0.0004, 0.0012, 0};
    inline const Model TOGETHER_RICURSIVE_AI_RIPPLE_65B_PREVIEW = {"ricursive-ai/Ripple-65B-Preview", Provider::Together, 0.0002, 0.0006, 0};
    inline const Model TOGETHER_THINKINGMACHINES_INKLING_2_SMALL = {"thinkingmachines/Inkling-2-Small", Provider::Together, 0.0003, 0.001, 0};
    inline const Model TOGETHER_ZAI_ORG_GLM_5_2 = {"zai-org/GLM-5.2", Provider::Together, 0.0014, 0.0044, 0};
    inline const Model TOGETHER_ZAI_ORG_GLM_5_3 = {"zai-org/GLM-5.3", Provider::Together, 0.0014, 0.0044, 0};
    inline const Model TOGETHER_ZAI_ORG_GLM_5_3_FLASH = {"zai-org/GLM-5.3-Flash", Provider::Together, 0.00015, 0.0005, 0};
    // Fireworks
    inline const Model FIREWORKS_EMBER_1        = {"accounts/fireworks/models/ember-1", Provider::Fireworks, 0.003, 0.015, 0};
    inline const Model FIREWORKS_MUSE_SPARK_OPEN = {"accounts/fireworks/models/muse-spark-open", Provider::Fireworks, 0.00125, 0.00425, 0};
    inline const Model FIREWORKS_KIMI_K3P1_PRO  = {"accounts/fireworks/models/kimi-k3p1-pro", Provider::Fireworks, 0.002, 0.01, 0};
    inline const Model FIREWORKS_KIMI_K3        = {"accounts/fireworks/models/kimi-k3", Provider::Fireworks, 0.003, 0.015, 0};
    inline const Model FIREWORKS_DEEPSEEK_V4P1_FLASH = {"accounts/fireworks/models/deepseek-v4p1-flash", Provider::Fireworks, 0.0003, 0.0012, 0};
    inline const Model FIREWORKS_DEEPSEEK_V4P1_FLASH_1017 = {"accounts/fireworks/models/deepseek-v4p1-flash-1017", Provider::Fireworks, 0.0003, 0.0012, 0};
    inline const Model FIREWORKS_GLM_5P3_FLASH  = {"accounts/fireworks/models/glm-5p3-flash", Provider::Fireworks, 0.00015, 0.0005, 0};
    inline const Model FIREWORKS_ULTRAMARINE_1_FLASH_US = {"accounts/fireworks/models/ultramarine-1-flash-us", Provider::Fireworks, 0.00015, 0.0005, 0};
    inline const Model FIREWORKS_GLM_5P3        = {"accounts/fireworks/models/glm-5p3", Provider::Fireworks, 0.0014, 0.0044, 0};
    inline const Model FIREWORKS_QWEN3P8_MAX    = {"accounts/fireworks/models/qwen3p8-max", Provider::Fireworks, 0.002, 0.006, 0};
    inline const Model FIREWORKS_MINIMAX_M3     = {"accounts/fireworks/models/minimax-m3", Provider::Fireworks, 0.0003, 0.0012, 0};
    inline const Model FIREWORKS_GPT_OSS_120B   = {"accounts/fireworks/models/gpt-oss-120b", Provider::Fireworks, 0.00015, 0.0006, 0};
    inline const Model FIREWORKS_NEMOTRON_LIGHTNING_3P5_30B_A3B = {"accounts/fireworks/models/nemotron-lightning-3p5-30b-a3b", Provider::Fireworks, 0.00005, 0.0002, 0};
    inline const Model FIREWORKS_NEMOTRON_3_ULTRA_NVFP4 = {"accounts/fireworks/models/nemotron-3-ultra-nvfp4", Provider::Fireworks, 0.0006, 0.0024, 0};
    // retired: no longer on the provider's pricing page; prices as last listed
    inline const Model CLAUDE_OPUS_4_1_20250805 = {"claude-opus-4-1-20250805", Provider::Anthropic, 0.015, 0.075, 0};
    inline const Model CLAUDE_OPUS_4_20250514   = {"claude-opus-4-20250514", Provider::Anthropic, 0.015, 0.075, 0};
    inline const Model CLAUDE_SONNET_4_20250514 = {"claude-sonnet-4-20250514", Provider::Anthropic, 0.003, 0.015, 0};
    inline const Model CLAUDE_3_5_HAIKU_20241022 = {"claude-3-5-haiku-20241022", Provider::Anthropic, 0.0008, 0.004, 0};
    inline const Model GPT_4_1_NANO             = {"gpt-4.1-nano", Provider::OpenAI, 0.0001, 0.0004, 0};
    inline const Model GPT_4O_2024_05_13        = {"gpt-4o-2024-05-13", Provider::OpenAI, 0.005, 0.015, 0};
    inline const Model O1                       = {"o1", Provider::OpenAI, 0.015, 0.06, 0};
    inline const Model O1_PRO                   = {"o1-pro", Provider::OpenAI, 0.15, 0.6, 0};
    inline const Model O4_MINI                  = {"o4-mini", Provider::OpenAI, 0.0011, 0.0044, 0};
    inline const Model O3_MINI                  = {"o3-mini", Provider::OpenAI, 0.0011, 0.0044, 0};
    inline const Model GPT4_TURBO               = {"gpt-4-turbo", Provider::OpenAI, 0.01, 0.03, 128000};
    inline const Model GPT_4_0613               = {"gpt-4-0613", Provider::OpenAI, 0.03, 0.06, 0};
    inline const Model GPT_3_5_TURBO            = {"gpt-3.5-turbo", Provider::OpenAI, 0.0005, 0.0015, 0};
    inline const Model GPT_3_5_TURBO_0125       = {"gpt-3.5-turbo-0125", Provider::OpenAI, 0.0005, 0.0015, 0};
    inline const Model GPT_5_5_CYBER            = {"gpt-5.5-cyber", Provider::OpenAI, 0.0125, 0.075, 0};
} // namespace models

namespace detail {
struct ModelAlias { const char* alias; const Model* model; };
inline const Model* const listed_models[] = {
    &models::CLAUDE_FABLE_5_2,
    &models::CLAUDE_FABLE_5_1,
    &models::CLAUDE_FABLE_5,
    &models::CLAUDE_OPUS_5_5,
    &models::CLAUDE_OPUS_5,
    &models::CLAUDE_OPUS_4_8,
    &models::CLAUDE_OPUS_4_7,
    &models::CLAUDE_OPUS_4_6,
    &models::CLAUDE_OPUS,
    &models::CLAUDE_SONNET_5_5,
    &models::CLAUDE_SONNET_5,
    &models::CLAUDE_SONNET_4_6,
    &models::CLAUDE_SONNET,
    &models::CLAUDE_HAIKU_5_5,
    &models::CLAUDE_HAIKU,
    &models::GPT6_ASTRA,
    &models::GPT_6_1_SOL,
    &models::GPT_6_1_TERRA,
    &models::GPT6_LUNA,
    &models::GPT6_SOL,
    &models::GPT_5_6_SOL,
    &models::GPT_5_6_TERRA,
    &models::GPT_5_6_LUNA,
    &models::GPT_5_5,
    &models::GPT_5_5_PRO,
    &models::GPT_5_4,
    &models::GPT_5_4_PRO,
    &models::GPT_5_4_MINI,
    &models::GPT_5_4_NANO,
    &models::GPT_5_2,
    &models::GPT_5_2_PRO,
    &models::GPT_5_1,
    &models::GPT_5,
    &models::GPT_5_MINI,
    &models::GPT_5_NANO,
    &models::GPT_5_PRO,
    &models::GPT_4_1,
    &models::GPT_4_1_MINI,
    &models::GPT4O,
    &models::GPT4O_MINI,
    &models::O3_PRO,
    &models::O3,
    &models::GPT_6_CYBER,
    &models::GPT_5_6_CYBER,
    &models::GPT_5_5_CYBER_PREVIEW,
    &models::GPT_6_ROSALIND,
    &models::GPT_ROSALIND_RESEARCH,
    &models::GPT_ROSALIND_DISCOVERY,
    &models::GPT_6_ASTRA_LAW,
    &models::CHAT_LATEST,
    &models::GPT_5_3_CODEX,
    &models::GPT_5_SEARCH_API,
    &models::TEXT_EMBEDDING_3_SMALL,
    &models::TEXT_EMBEDDING_3_LARGE,
    &models::TEXT_EMBEDDING_ADA_002,
    &models::GEMINI_4_CARBON,
    &models::GEMINI_4_ARGON,
    &models::GEMINI_4_HYDROGEN,
    &models::GEMINI_3_8_FLASH,
    &models::GEMINI_3_6_FLASH,
    &models::GEMINI_3_5_FLASH_LITE,
    &models::GEMINI_3_1_FLASH_LITE,
    &models::GEMINI_3_1_PRO_PREVIEW,
    &models::GEMINI_3_FLASH_PREVIEW,
    &models::GEMINI_2_5_PRO,
    &models::GEMINI_2_5_FLASH,
    &models::GEMINI_2_5_FLASH_LITE,
    &models::DEEPSEEK_FLASH,
    &models::DEEPSEEK_V4_PRO,
    &models::MISTRAL_LARGE_4,
    &models::MISTRAL_SMALL_2603,
    &models::MINISTRAL_14B_2512,
    &models::MINISTRAL_8B_2512,
    &models::MINISTRAL_3B_2512,
    &models::CODESTRAL_2508,
    &models::ZAI_GLM_5_3,
    &models::ZAI_GLM_5_3_FLASH,
    &models::DEEPSEEK_V4_1_FLASH_1017,
    &models::MISTRAL_EMBED_2312,
    &models::CODESTRAL_EMBED_2505,
    &models::GROK_4_7,
    &models::GROK_4_6,
    &models::GROK_4_5,
    &models::GROK_4_3,
    &models::GROK_4_20_0309_REASONING,
    &models::GROK_4_20_0309_NON_REASONING,
    &models::GROK_4_20_MULTI_AGENT_0309,
    &models::GROK_BUILD_0_1,
    &models::COMMAND_R_PLUS_08_2024,
    &models::GROQ_OPENAI_GPT_OSS_120B,
    &models::GROQ_OPENAI_GPT_OSS_20B,
    &models::GROQ_OPENAI_GPT_OSS_SAFEGUARD_20B,
    &models::GROQ_QWEN_QWEN3_8_27B,
    &models::GROQ_META_LLAMA_LLAMA_PROMPT_GUARD_2_22M,
    &models::GROQ_META_LLAMA_LLAMA_PROMPT_GUARD_2_86M,
    &models::TOGETHER_DEEPSEEK_AI_DEEPSEEK_V4_PRO_0813,
    &models::TOGETHER_DEEPSEEK_AI_DEEPSEEK_V4_1_FLASH,
    &models::TOGETHER_DEEPSEEK_AI_DEEPSEEK_V4_1_FLASH_1017,
    &models::TOGETHER_ELORIAN_LABS_GLIMPSE_1_0,
    &models::TOGETHER_META_MODELS_MUSE_GLIMMER_30B,
    &models::TOGETHER_META_MODELS_MUSE_SPARK_OPEN_816B_A42B,
    &models::TOGETHER_MINIMAXAI_MINIMAX_M3_1_FLASH,
    &models::TOGETHER_MOONSHOTAI_KIMI_K3,
    &models::TOGETHER_MOONSHOTAI_KIMI_K3_1_PRO,
    &models::TOGETHER_NVIDIA_NVIDIA_NEMOTRON_3_5_SUPER_VL,
    &models::TOGETHER_OPENAI_GPT_OSS_120B,
    &models::TOGETHER_QWEN_QWEN3_5_9B,
    &models::TOGETHER_QWEN_QWEN3_6_PLUS,
    &models::TOGETHER_QWEN_QWEN3_7_MAX,
    &models::TOGETHER_QWEN_QWEN3_7_PLUS,
    &models::TOGETHER_QWEN_QWEN3_8_FLASH,
    &models::TOGETHER_REFLECTION_BEAM_1_0_501B_A23B,
    &models::TOGETHER_RICURSIVE_AI_RIPPLE_65B_PREVIEW,
    &models::TOGETHER_THINKINGMACHINES_INKLING_2_SMALL,
    &models::TOGETHER_ZAI_ORG_GLM_5_2,
    &models::TOGETHER_ZAI_ORG_GLM_5_3,
    &models::TOGETHER_ZAI_ORG_GLM_5_3_FLASH,
    &models::FIREWORKS_EMBER_1,
    &models::FIREWORKS_MUSE_SPARK_OPEN,
    &models::FIREWORKS_KIMI_K3P1_PRO,
    &models::FIREWORKS_KIMI_K3,
    &models::FIREWORKS_DEEPSEEK_V4P1_FLASH,
    &models::FIREWORKS_DEEPSEEK_V4P1_FLASH_1017,
    &models::FIREWORKS_GLM_5P3_FLASH,
    &models::FIREWORKS_ULTRAMARINE_1_FLASH_US,
    &models::FIREWORKS_GLM_5P3,
    &models::FIREWORKS_QWEN3P8_MAX,
    &models::FIREWORKS_MINIMAX_M3,
    &models::FIREWORKS_GPT_OSS_120B,
    &models::FIREWORKS_NEMOTRON_LIGHTNING_3P5_30B_A3B,
    &models::FIREWORKS_NEMOTRON_3_ULTRA_NVFP4,
};
inline const Model* const retired_models[] = {
    &models::CLAUDE_OPUS_4_1_20250805,
    &models::CLAUDE_OPUS_4_20250514,
    &models::CLAUDE_SONNET_4_20250514,
    &models::CLAUDE_3_5_HAIKU_20241022,
    &models::GPT_4_1_NANO,
    &models::GPT_4O_2024_05_13,
    &models::O1,
    &models::O1_PRO,
    &models::O4_MINI,
    &models::O3_MINI,
    &models::GPT4_TURBO,
    &models::GPT_4_0613,
    &models::GPT_3_5_TURBO,
    &models::GPT_3_5_TURBO_0125,
    &models::GPT_5_5_CYBER,
};
inline const ModelAlias model_aliases[] = {
    {"claude-opus-4-5-20251101", &models::CLAUDE_OPUS},
    {"claude-sonnet-4-5-20250929", &models::CLAUDE_SONNET},
    {"claude-haiku-4-5-20251001", &models::CLAUDE_HAIKU},
    {"gpt-4-turbo-2024-04-09", &models::GPT4_TURBO},
    {"deepseek-v4-flash", &models::DEEPSEEK_FLASH},
    {"deepseek-v4-flash-vision-exp", &models::DEEPSEEK_FLASH},
    {"deepseek-chat", &models::DEEPSEEK_FLASH},
    {"deepseek-reasoner", &models::DEEPSEEK_FLASH},
    {"mistral-large-4-0", &models::MISTRAL_LARGE_4},
    {"mistral-large-latest", &models::MISTRAL_LARGE_4},
    {"mistral-small-latest", &models::MISTRAL_SMALL_2603},
    {"ministral-14b-latest", &models::MINISTRAL_14B_2512},
    {"ministral-8b-latest", &models::MINISTRAL_8B_2512},
    {"ministral-3b-latest", &models::MINISTRAL_3B_2512},
    {"codestral-latest", &models::CODESTRAL_2508},
    {"zai-glm-5", &models::ZAI_GLM_5_3},
    {"zai-glm-latest", &models::ZAI_GLM_5_3},
    {"mistral-embed", &models::MISTRAL_EMBED_2312},
    {"codestral-embed", &models::CODESTRAL_EMBED_2505},
};
} // namespace detail
// END GENERATED MODELS

/// Result of a token counting operation.
struct TokenCount {
    size_t      tokens;                 ///< Estimated token count
    size_t      characters;            ///< Raw character count
    double      estimated_cost_usd;    ///< Input cost only (output unknown pre-call)
    bool        exceeds_context;       ///< True if tokens > model.context_window
    std::string model_name;
};

/// One row in a cross-model cost comparison.
struct CostComparison {
    std::string model_name;
    size_t      tokens;
    double      input_cost_usd;
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Estimate token count and input cost for a single text string.
///
/// Uses a cl100k_base heuristic: ~4 chars/token for prose, adjusted for
/// code, numbers, and non-ASCII. Accuracy is approximately ±5% vs tiktoken.
///
/// # Arguments
/// * `text`: The string to analyse
/// * `model`: Model to price against
///
/// # Panics
/// This function never panics.
TokenCount count(const std::string& text, const Model& model);

/// Estimate token count for a chat-format message list.
/// Adds per-message overhead (role tokens + 4 tokens/message, as OpenAI does).
///
/// # Arguments
/// * `messages`: Vector of {role, content} pairs
/// * `model`: Model to price against
///
/// # Panics
/// This function never panics.
TokenCount count_messages(
    const std::vector<std::pair<std::string, std::string>>& messages,
    const Model& model
);

/// Throw std::runtime_error if estimated input cost exceeds budget_usd.
/// Call this before making an LLM API call to enforce cost limits.
///
/// # Throws
/// `std::runtime_error` if tc.estimated_cost_usd > budget_usd
void assert_budget(const TokenCount& tc, double budget_usd);

/// Compare input cost across every listed built-in chat model (all_models()
/// minus the embedding models) for the same text.
/// Results are sorted cheapest-first; ties keep table order.
///
/// # Arguments
/// * `text`: The prompt to price across all models
///
/// # Panics
/// This function never panics.
std::vector<CostComparison> compare_costs(const std::string& text);

/// Compare input cost across the given models, sorted cheapest-first.
std::vector<CostComparison> compare_costs(const std::string& text,
                                          const std::vector<Model>& models);

/// Every built-in model the providers still list, in table order.
/// Retired models are left out but still found by find_model().
std::vector<const Model*> all_models();

/// Look up a built-in model by API id or alias (ASCII case-insensitive),
/// for example "claude-sonnet-4-5-20250929", "deepseek-chat" or
/// "accounts/fireworks/models/kimi-k3". Returns nullptr when unknown.
/// Ids served by several hosts (e.g. "openai/gpt-oss-120b") return the first
/// match; pass the provider to pick one.
const Model* find_model(std::string_view name);
const Model* find_model(std::string_view name, Provider provider);

/// Format a USD cost as a human-readable string.
/// Values >= $0.01 → "$0.0123"
/// Values <  $0.01 → "0.12¢"
/// Values == $0.00 → "$0.0000"
///
/// # Panics
/// This function never panics.
std::string format_cost(double usd);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------
#ifdef LLM_COST_IMPLEMENTATION

namespace llm {
namespace detail {

// ---------------------------------------------------------------------------
// cl100k_base token count approximation.
//
// Strategy (mirrors tiktoken's rough behaviour without the BPE table):
//   1. Split on whitespace boundaries.
//   2. For each "word", estimate sub-tokens:
//      - Pure ASCII alphabetic:   1 token per ~4 chars (English prose)
//      - Numeric runs:            1 token per ~3 chars
//      - Punctuation / symbols:   1 token each (usually)
//      - Non-ASCII (unicode):     1 token per ~2 bytes (conservative)
//   3. Add a fixed overhead for leading whitespace tokens.
//
// Typical accuracy: ±5% vs tiktoken on English prose, ±10% on mixed code.
// ---------------------------------------------------------------------------
static size_t estimate_tokens(std::string_view text) {
    if (text.empty()) return 0;

    size_t tokens = 0;
    size_t i      = 0;
    const size_t n = text.size();

    while (i < n) {
        unsigned char c = static_cast<unsigned char>(text[i]);

        // Whitespace: each run of whitespace is ~1 token
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            ++tokens;
            while (i < n) {
                unsigned char wc = static_cast<unsigned char>(text[i]);
                if (wc != ' ' && wc != '\t' && wc != '\n' && wc != '\r') break;
                ++i;
            }
            continue;
        }

        // Numeric run: digits encode at ~3 chars/token
        if (c >= '0' && c <= '9') {
            size_t start = i;
            while (i < n) {
                unsigned char dc = static_cast<unsigned char>(text[i]);
                if (dc < '0' || dc > '9') break;
                ++i;
            }
            size_t len = i - start;
            tokens += (len + 2) / 3; // ceil(len/3)
            continue;
        }

        // ASCII alphabetic run: ~4 chars/token
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            size_t start = i;
            while (i < n) {
                unsigned char ac = static_cast<unsigned char>(text[i]);
                if (!((ac >= 'a' && ac <= 'z') ||
                      (ac >= 'A' && ac <= 'Z') ||
                      (ac >= '0' && ac <= '9') ||
                       ac == '_'))
                    break;
                ++i;
            }
            size_t len = i - start;
            tokens += (len + 3) / 4; // ceil(len/4)
            continue;
        }

        // Non-ASCII (multi-byte UTF-8): conservative, 1 token per 2 bytes
        if (c >= 0x80) {
            size_t start = i;
            while (i < n && static_cast<unsigned char>(text[i]) >= 0x80) ++i;
            size_t bytes = i - start;
            tokens += (bytes + 1) / 2;
            continue;
        }

        // Single ASCII punctuation / symbol: 1 token each
        ++tokens;
        ++i;
    }

    // Apply a small empirical correction factor (tiktoken merges some pairs)
    // Multiply by 0.85 to account for BPE merges that our heuristic misses.
    tokens = static_cast<size_t>(std::ceil(tokens * 0.85));

    return tokens == 0 ? 1 : tokens;
}

static bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x = static_cast<char>(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = static_cast<char>(y - 'A' + 'a');
        if (x != y) return false;
    }
    return true;
}

static const Model* find(std::string_view name, const Provider* provider) {
    auto ok = [&](const Model* m) { return !provider || m->provider == *provider; };
    for (const Model* m : listed_models)
        if (ok(m) && iequals(m->name, name)) return m;
    for (const Model* m : retired_models)
        if (ok(m) && iequals(m->name, name)) return m;
    for (const ModelAlias& a : model_aliases)
        if (ok(a.model) && iequals(a.alias, name)) return a.model;
    return nullptr;
}

static std::vector<CostComparison> compare(const std::string& text,
                                           const Model* const* first,
                                           const Model* const* last,
                                           bool skip_embeddings) {
    const size_t tok = estimate_tokens(text);
    std::vector<CostComparison> results;
    results.reserve(static_cast<size_t>(last - first));
    for (; first != last; ++first) {
        const Model& m = **first;
        if (skip_embeddings && m.output_cost_per_1k == 0.0) continue;
        CostComparison cc;
        cc.model_name     = m.name;
        cc.tokens         = tok;
        cc.input_cost_usd = (tok / 1000.0) * m.input_cost_per_1k;
        results.push_back(cc);
    }
    std::stable_sort(results.begin(), results.end(),
        [](const CostComparison& a, const CostComparison& b) {
            return a.input_cost_usd < b.input_cost_usd;
        });
    return results;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Public API implementations
// ---------------------------------------------------------------------------

TokenCount count(const std::string& text, const Model& model) {
    const size_t tokens    = detail::estimate_tokens(text);
    const double cost      = (tokens / 1000.0) * model.input_cost_per_1k;
    const bool   exceeds   = model.context_window != 0 && tokens > model.context_window;

    TokenCount tc;
    tc.tokens              = tokens;
    tc.characters          = text.size();
    tc.estimated_cost_usd  = cost;
    tc.exceeds_context     = exceeds;
    tc.model_name          = model.name;
    return tc;
}

TokenCount count_messages(
    const std::vector<std::pair<std::string, std::string>>& messages,
    const Model& model)
{
    // OpenAI overhead: 4 tokens per message, 3 tokens for role, +3 for reply priming
    size_t tokens = 3; // reply priming
    for (const auto& [role, content] : messages) {
        tokens += 4; // per-message overhead
        tokens += detail::estimate_tokens(role);
        tokens += detail::estimate_tokens(content);
    }

    const double cost    = (tokens / 1000.0) * model.input_cost_per_1k;
    const bool   exceeds = model.context_window != 0 && tokens > model.context_window;

    // Compute total chars for reference
    size_t chars = 0;
    for (const auto& [role, content] : messages)
        chars += role.size() + content.size();

    TokenCount tc;
    tc.tokens             = tokens;
    tc.characters         = chars;
    tc.estimated_cost_usd = cost;
    tc.exceeds_context    = exceeds;
    tc.model_name         = model.name;
    return tc;
}

void assert_budget(const TokenCount& tc, double budget_usd) {
    if (tc.estimated_cost_usd > budget_usd) {
        std::ostringstream oss;
        oss << "Budget exceeded: estimated " << format_cost(tc.estimated_cost_usd)
            << " > limit " << format_cost(budget_usd)
            << " (" << tc.tokens << " tokens on " << tc.model_name << ")";
        throw std::runtime_error(oss.str());
    }
}

std::vector<CostComparison> compare_costs(const std::string& text) {
    return detail::compare(text, std::begin(detail::listed_models),
                           std::end(detail::listed_models), true);
}

std::vector<CostComparison> compare_costs(const std::string& text,
                                          const std::vector<Model>& models) {
    std::vector<const Model*> ptrs;
    ptrs.reserve(models.size());
    for (const Model& m : models) ptrs.push_back(&m);
    return detail::compare(text, ptrs.data(), ptrs.data() + ptrs.size(), false);
}

std::vector<const Model*> all_models() {
    return std::vector<const Model*>(std::begin(detail::listed_models),
                                     std::end(detail::listed_models));
}

const Model* find_model(std::string_view name) {
    return detail::find(name, nullptr);
}

const Model* find_model(std::string_view name, Provider provider) {
    return detail::find(name, &provider);
}

std::string format_cost(double usd) {
    if (usd == 0.0) {
        return "$0.0000";
    }
    std::ostringstream oss;
    if (usd >= 0.01) {
        oss << "$" << std::fixed << std::setprecision(4) << usd;
    } else {
        // Convert to cents
        oss << std::fixed << std::setprecision(4) << (usd * 100.0) << "\xC2\xA2"; // UTF-8 ¢
    }
    return oss.str();
}

} // namespace llm

#endif // LLM_COST_IMPLEMENTATION
