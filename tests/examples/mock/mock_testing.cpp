#define LLM_MOCK_IMPLEMENTATION
#include "llm_mock.hpp"
#include <cassert>
#include <iostream>

int main() {
    // Scripted responses
    {
        llm::MockConfig cfg;
        cfg.strategy = llm::ScriptedResponses{{"Hello!", "How can I help?", "Goodbye!"}, 0};
        llm::MockLLM mock(cfg);
        assert(mock.complete("hi") == "Hello!");
        assert(mock.complete("?")  == "How can I help?");
        assert(mock.complete("bye") == "Goodbye!");
        assert(mock.complete("x")   == "Hello!"); // wraps
        std::cout << "Scripted: OK (" << mock.call_count() << " calls)\n";
    }

    // Pattern matching
    {
        llm::MockConfig cfg;
        llm::PatternResponses pr;
        pr.patterns = {{"weather", "It is sunny!"}, {"time", "It is noon."}};
        pr.fallback = "I don't know.";
        cfg.strategy = pr;
        llm::MockLLM mock(cfg);
        assert(mock.complete("What is the weather?") == "It is sunny!");
        assert(mock.complete("What time is it?")     == "It is noon.");
        assert(mock.complete("Who are you?")         == "I don't know.");
        std::cout << "Pattern: OK\n";
    }

    // Echo response
    {
        llm::MockConfig cfg;
        cfg.strategy = llm::EchoResponse{">> "};
        llm::MockLLM mock(cfg);
        std::string r = mock.complete("test");
        assert(r == ">> test");
        std::cout << "Echo: OK — '" << r << "'\n";
    }

    // Streaming simulation
    {
        llm::MockConfig cfg;
        cfg.strategy         = llm::EchoResponse{"Reply: "};
        cfg.stream_sim       = true;
        cfg.latency_ms       = 0.0;
        cfg.tokens_per_chunk = 2;
        llm::MockLLM mock(cfg);
        std::string full;
        std::vector<std::string> chunks;
        mock.complete_stream("one two three four", [&](const std::string& d){
            chunks.push_back(d);
            full += d + " ";
        });
        std::cout << "Streaming chunks: " << chunks.size() << ", full: '" << full << "'\n";
    }

    // Global mock slot
    llm::install_mock(llm::MockConfig{llm::EchoResponse{"MOCK:"}, 0.0});
    assert(llm::is_mock_installed());
    std::cout << "Global mock: " << llm::call_mock("ping") << "\n";
    llm::uninstall_mock();
    assert(!llm::is_mock_installed());

    std::cout << "All mock tests passed.\n";
    return 0;
}
