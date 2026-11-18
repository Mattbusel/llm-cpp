// Offline test of llm::SseParser / llm::parse_sse. Builds with LLM_STREAM_NO_CURL,
// so it also proves the parser needs neither libcurl nor LLM_STREAM_IMPLEMENTATION.
// No network and no API key.
#define LLM_STREAM_NO_CURL
#include "llm_stream.hpp"

#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                \
        }                                                              \
    } while (0)

// Feed `body` in chunks of `chunk` bytes, then finish.
static std::vector<llm::SseEvent> chunked(const std::string& body, size_t chunk) {
    std::vector<llm::SseEvent> out;
    llm::SseParser p;
    const llm::SseCallback collect = [&out](const llm::SseEvent& e) { out.push_back(e); };
    for (size_t i = 0; i < body.size(); i += chunk)
        p.feed(std::string_view(body).substr(i, chunk), collect);
    p.finish(collect);
    return out;
}

int main() {
    // Anthropic Messages stream: several events, any chunking gives the same events.
    const std::string anthropic =
        "event: message_start\n"
        "data: {\"type\":\"message_start\",\"message\":{\"usage\":{\"input_tokens\":9}}}\n\n"
        "event: ping\n"
        "data: {\"type\": \"ping\"}\n\n"
        "event: content_block_delta\n"
        "data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hi\"}}\n\n"
        "event: message_delta\n"
        "data: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"end_turn\"},\"usage\":{\"output_tokens\":2}}\n\n"
        "event: message_stop\n"
        "data: {\"type\":\"message_stop\"}\n\n";
    const auto whole = llm::parse_sse(anthropic);
    CHECK(whole.size() == 5);
    if (whole.size() == 5) {
        CHECK(whole[0].event == "message_start");
        CHECK(whole[2].event == "content_block_delta");
        CHECK(whole[2].data.find("\"text\":\"Hi\"") != std::string::npos);
        CHECK(whole[4].event == "message_stop");
        CHECK(whole[4].data == "{\"type\":\"message_stop\"}");
    }
    for (size_t chunk = 1; chunk <= anthropic.size(); ++chunk) {
        const auto ev = chunked(anthropic, chunk);
        CHECK(ev.size() == whole.size());
        for (size_t i = 0; i < ev.size() && i < whole.size(); ++i) {
            CHECK(ev[i].event == whole[i].event);
            CHECK(ev[i].data == whole[i].data);
        }
    }

    // CRLF and lone CR line ends, including a CRLF split across two chunks.
    for (const char* eol : {"\r\n", "\r"}) {
        const std::string e(eol);
        const std::string body = "event: a" + e + "data: 1" + e + e + "data: 2" + e + e;
        for (size_t chunk = 1; chunk <= body.size(); ++chunk) {
            const auto ev = chunked(body, chunk);
            CHECK(ev.size() == 2);
            if (ev.size() == 2) {
                CHECK(ev[0].event == "a");
                CHECK(ev[0].data == "1");
                CHECK(ev[1].event == "message");
                CHECK(ev[1].data == "2");
            }
        }
    }

    // Multi-line data, comments, no space after the colon, a field with no colon,
    // extra spaces kept, unknown fields ignored, leading BOM.
    {
        const auto ev = llm::parse_sse(
            "\xEF\xBB\xBF: keep-alive comment\n"
            "event:update\n"
            "data:first\n"
            "data:  second\n"
            "data\n"
            "foo: bar\n\n");
        CHECK(ev.size() == 1);
        if (ev.size() == 1) {
            CHECK(ev[0].event == "update");
            CHECK(ev[0].data == "first\n second\n");
        }
    }

    // An event with no data is dropped and its event type does not leak into the next.
    {
        const auto ev = llm::parse_sse("event: lonely\n\ndata: x\n\n");
        CHECK(ev.size() == 1);
        if (ev.size() == 1) {
            CHECK(ev[0].event == "message");
            CHECK(ev[0].data == "x");
        }
    }

    // An empty data line still dispatches, with empty data.
    {
        const auto ev = llm::parse_sse("data:\n\n");
        CHECK(ev.size() == 1);
        if (ev.size() == 1) CHECK(ev[0].data.empty());
    }

    // id carries over to later events; retry is read when all digits.
    {
        llm::SseParser p;
        std::vector<llm::SseEvent> ev;
        const llm::SseCallback collect = [&ev](const llm::SseEvent& e) { ev.push_back(e); };
        p.feed("id: 7\nretry: 1500\ndata: a\n\ndata: b\n\nretry: 2x\n", collect);
        CHECK(ev.size() == 2);
        if (ev.size() == 2) {
            CHECK(ev[0].id == "7");
            CHECK(ev[1].id == "7");
        }
        CHECK(p.retry_ms() == 1500);
    }

    // finish() flushes a last event with no blank line, and a last line with no line end.
    {
        const auto a = llm::parse_sse("event: message_stop\ndata: {\"type\":\"message_stop\"}\n");
        CHECK(a.size() == 1);
        if (a.size() == 1) CHECK(a[0].event == "message_stop");
        const auto b = llm::parse_sse("data: tail");
        CHECK(b.size() == 1);
        if (b.size() == 1) CHECK(b[0].data == "tail");
        CHECK(llm::parse_sse("").empty());
        CHECK(llm::parse_sse("\n\n: only a comment\n\n").empty());
    }

    if (failures == 0) std::printf("test_sse: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
