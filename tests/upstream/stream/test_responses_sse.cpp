// Offline test of the Responses API stream parser: feeds recorded-shape SSE bytes to the
// curl write callback in uneven chunks and checks the text, usage, id and error paths.
// No network and no API key.
#define LLM_STREAM_IMPLEMENTATION
#include "llm_stream.hpp"

#include <cstdio>
#include <cstring>
#include <string>

static int failures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                \
        }                                                              \
    } while (0)

static void feed(llm::detail::StreamCtx& ctx, const std::string& body, size_t chunk) {
    for (size_t i = 0; i < body.size(); i += chunk) {
        std::string part = body.substr(i, chunk);
        llm::detail::write_callback(&part[0], 1, part.size(), &ctx);
    }
}

int main() {
    const std::string ok =
        "event: response.created\n"
        "data: {\"type\":\"response.created\",\"response\":{\"id\":\"resp_abc\",\"status\":\"in_progress\"}}\n\n"
        "event: response.output_text.delta\n"
        "data: {\"type\":\"response.output_text.delta\",\"item_id\":\"msg_1\",\"output_index\":0,\"content_index\":0,\"delta\":\"Hello\"}\n\n"
        "event: response.output_text.delta\r\n"
        "data: {\"type\":\"response.output_text.delta\",\"item_id\":\"msg_1\",\"output_index\":0,\"content_index\":0,\"delta\":\", \\\"world\\\"\\n\"}\r\n\r\n"
        "event: response.output_text.done\n"
        "data: {\"type\":\"response.output_text.done\",\"text\":\"Hello, \\\"world\\\"\\n\"}\n\n"
        "event: response.completed\n"
        "data: {\"type\":\"response.completed\",\"response\":{\"id\":\"resp_abc\",\"status\":\"completed\","
        "\"usage\":{\"input_tokens\":12,\"input_tokens_details\":{\"cached_tokens\":0},\"output_tokens\":5,\"total_tokens\":17}}}\n\n";

    for (size_t chunk : {static_cast<size_t>(1), static_cast<size_t>(7), static_cast<size_t>(4096)}) {
        std::string text;
        llm::detail::StreamCtx ctx;
        ctx.provider = llm::detail::Provider::OpenAIResponses;
        ctx.on_token = [&](std::string_view t) { text += t; };
        feed(ctx, ok, chunk);
        CHECK(text == "Hello, \"world\"\n");
        CHECK(ctx.token_count == 2);
        CHECK(ctx.completed);
        CHECK(ctx.api_error.empty());
        CHECK(ctx.input_tokens == 12);
        CHECK(ctx.output_tokens == 5);
        CHECK(ctx.response_id == "resp_abc");
    }

    const std::string failed =
        "event: response.output_text.delta\n"
        "data: {\"type\":\"response.output_text.delta\",\"delta\":\"partial\"}\n\n"
        "event: response.failed\n"
        "data: {\"type\":\"response.failed\",\"response\":{\"id\":\"resp_x\",\"status\":\"failed\","
        "\"error\":{\"code\":\"server_error\",\"message\":\"The model failed to respond.\"}}}\n\n";
    {
        llm::detail::StreamCtx ctx;
        ctx.provider = llm::detail::Provider::OpenAIResponses;
        feed(ctx, failed, 5);
        CHECK(!ctx.completed);
        CHECK(ctx.api_error == "The model failed to respond.");
    }

    const std::string error_event =
        "event: error\n"
        "data: {\"type\":\"error\",\"code\":\"rate_limit_exceeded\",\"message\":\"Rate limit reached.\",\"param\":null}\n\n";
    {
        llm::detail::StreamCtx ctx;
        ctx.provider = llm::detail::Provider::OpenAIResponses;
        feed(ctx, error_event, 3);
        CHECK(ctx.api_error == "Rate limit reached.");
    }

    const std::string incomplete =
        "data: {\"type\":\"response.incomplete\",\"response\":{\"incomplete_details\":{\"reason\":\"max_output_tokens\"}}}\n\n";
    {
        llm::detail::StreamCtx ctx;
        ctx.provider = llm::detail::Provider::OpenAIResponses;
        feed(ctx, incomplete, 4096);
        CHECK(ctx.api_error == "response incomplete: max_output_tokens");
    }

    // Request bodies.
    llm::Config cfg;
    cfg.model = "gpt-4.1-mini";
    cfg.max_tokens = 64;
    cfg.system_prompt = "Be brief.";
    std::string body = llm::detail::build_responses_body("Say \"hi\"", cfg);
    CHECK(body.find("\"input\":\"Say \\\"hi\\\"\"") != std::string::npos);
    CHECK(body.find("\"instructions\":\"Be brief.\"") != std::string::npos);
    CHECK(body.find("\"max_output_tokens\":64") != std::string::npos);
    CHECK(body.find("\"stream\":true") != std::string::npos);
    CHECK(body.find("\"temperature\"") != std::string::npos);
    cfg.model = "o4-mini";
    CHECK(llm::detail::build_responses_body("x", cfg).find("temperature") == std::string::npos);
    cfg.model = "gpt-5-mini";
    CHECK(llm::detail::build_responses_body("x", cfg).find("temperature") == std::string::npos);

    // Chat Completions parsing is unchanged.
    {
        std::string text;
        llm::detail::StreamCtx ctx;
        ctx.provider = llm::detail::Provider::OpenAI;
        ctx.on_token = [&](std::string_view t) { text += t; };
        feed(ctx,
             "data: {\"choices\":[{\"delta\":{\"content\":\"a\"}}]}\n\n"
             "data: {\"choices\":[{\"delta\":{\"content\":\"b\"}}]}\n\ndata: [DONE]\n\n",
             6);
        CHECK(text == "ab");
    }

    // Anthropic Messages parsing: text deltas only, ping/message_stop ignored, any chunking.
    for (size_t chunk : {static_cast<size_t>(1), static_cast<size_t>(5), static_cast<size_t>(4096)}) {
        std::string text;
        llm::detail::StreamCtx ctx;
        ctx.provider = llm::detail::Provider::Anthropic;
        ctx.on_token = [&](std::string_view t) { text += t; };
        feed(ctx,
             "event: message_start\r\n"
             "data: {\"type\":\"message_start\",\"message\":{\"usage\":{\"input_tokens\":3}}}\r\n\r\n"
             "event: ping\ndata: {\"type\": \"ping\"}\n\n"
             "event: content_block_delta\n"
             "data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hel\"}}\n\n"
             "event: content_block_delta\n"
             "data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"lo\"}}\n\n"
             "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n",
             chunk);
        CHECK(text == "Hello");
        CHECK(ctx.token_count == 2);
    }

    // A last event with no closing blank line is delivered when the stream ends.
    {
        std::string text;
        llm::detail::StreamCtx ctx;
        ctx.provider = llm::detail::Provider::OpenAI;
        ctx.on_token = [&](std::string_view t) { text += t; };
        feed(ctx, "data: {\"choices\":[{\"delta\":{\"content\":\"z\"}}]}\n", 4096);
        CHECK(text.empty());
        llm::detail::finish_stream(&ctx);
        CHECK(text == "z");
    }

    if (failures == 0) std::printf("test_responses_sse: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
