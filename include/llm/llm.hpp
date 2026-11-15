#pragma once

// llm.hpp: umbrella header for llm-cpp.
//
// Includes the declarations of every llm-cpp library. It does not pull in any
// implementation: in exactly ONE .cpp file of your program, define the
// implementation macro of each library you use before including its header,
// for example
//
//     #define LLM_CACHE_IMPLEMENTATION
//     #include <llm/llm_cache.hpp>
//
// llm_eval.hpp and llm_stream.hpp include <curl/curl.h> in their declarations,
// so this header only includes them when libcurl's headers are on the include
// path. Link llm::llm (or the per-library targets) with CMake to get the right
// include paths and libraries.

#define LLM_CPP_VERSION_MAJOR 1
#define LLM_CPP_VERSION_MINOR 1
#define LLM_CPP_VERSION_PATCH 0
#define LLM_CPP_VERSION "1.1.0"

// No external dependencies.
#include "llm_cache.hpp"
#include "llm_compress.hpp"
#include "llm_cost.hpp"
#include "llm_format.hpp"
#include "llm_guard.hpp"
#include "llm_json.hpp"
#include "llm_log.hpp"
#include "llm_mock.hpp"
#include "llm_parse.hpp"
#include "llm_pool.hpp"
#include "llm_retry.hpp"
#include "llm_router.hpp"
#include "llm_template.hpp"
#include "llm_trace.hpp"

// Implementations need libcurl; declarations do not.
#include "llm_ab.hpp"
#include "llm_agent.hpp"
#include "llm_audio.hpp"
#include "llm_batch.hpp"
#include "llm_chat.hpp"
#include "llm_embed.hpp"
#include "llm_finetune.hpp"
#include "llm_rag.hpp"
#include "llm_rank.hpp"
#include "llm_vision.hpp"

// Declarations include <curl/curl.h>.
#if defined(__has_include)
#  if __has_include(<curl/curl.h>)
#    include "llm_eval.hpp"
#    include "llm_stream.hpp"
#  endif
#endif
