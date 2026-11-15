#define LLM_AGENT_IMPLEMENTATION
#include "llm_agent.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }

    std::vector<llm::Tool> tools = {
        {
            "read_file", "Read the contents of a file",
            {"path"}, {"File path to read"},
            [](std::map<std::string,std::string> args) -> std::string {
                std::ifstream f(args["path"]);
                if (!f) return "[error] cannot open: " + args["path"];
                std::ostringstream ss; ss << f.rdbuf();
                return ss.str();
            }
        },
        {
            "write_file", "Write content to a file",
            {"path","content"}, {"File path","Content to write"},
            [](std::map<std::string,std::string> args) -> std::string {
                std::ofstream f(args["path"]);
                if (!f) return "[error] cannot write: " + args["path"];
                f << args["content"];
                return "Written " + std::to_string(args["content"].size()) + " bytes to " + args["path"];
            }
        },
        {
            "list_dir", "List files in a directory",
            {"path"}, {"Directory path"},
            [](std::map<std::string,std::string> args) -> std::string {
                std::string result;
                try {
                    for (const auto& e : std::filesystem::directory_iterator(args["path"]))
                        result += e.path().filename().string() + "\n";
                } catch (...) { return "[error] cannot list: " + args["path"]; }
                return result.empty() ? "(empty)" : result;
            }
        }
    };

    llm::AgentConfig cfg;
    cfg.api_key   = key;
    cfg.model     = "gpt-4o-mini";
    cfg.verbose   = true;
    cfg.system_prompt = "You are a file system assistant. Use tools to read, write, and list files.";

    std::string prompt = "List the files in the current directory (use path '.'), then write a file called 'agent_output.txt' with the message 'Hello from the agent!', then read it back to confirm.";
    std::cout << "Prompt: " << prompt << "\n\n";

    auto result = llm::run_agent(prompt, tools, cfg);

    std::cout << "\nAnswer: " << result.answer << "\n";
    std::cout << "Iterations: " << result.iterations_used << "\n";

    // cleanup
    std::remove("agent_output.txt");
    return 0;
}
