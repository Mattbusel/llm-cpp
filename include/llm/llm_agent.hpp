#pragma once
// llm-agent: single-header C++ tool-calling agent loop
// #define LLM_AGENT_IMPLEMENTATION in ONE .cpp before including.

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace llm {

struct Tool {
    std::string name;
    std::string description;
    std::vector<std::string> param_names;
    std::vector<std::string> param_descriptions;
    std::function<std::string(std::map<std::string, std::string>)> fn;
};

struct AgentConfig {
    std::string api_key;
    std::string model          = "gpt-4o-mini";
    std::string api_url        = "https://api.openai.com/v1/chat/completions";
    int         max_iterations = 10;
    long        timeout_secs   = 60;
    bool        verbose        = false;
    std::string system_prompt;
};

struct AgentStep {
    enum class Type { ToolCall, FinalAnswer };
    Type        type;
    std::string tool_name;
    std::map<std::string, std::string> tool_args;
    std::string tool_result;
    std::string content;
};

struct AgentResult {
    std::string            answer;
    std::vector<AgentStep> steps;
    int                    iterations_used;
    bool                   max_iterations_reached;
};

AgentResult run_agent(
    const std::string&        prompt,
    const std::vector<Tool>&  tools,
    const AgentConfig&        config
);

} // namespace llm

#ifdef LLM_AGENT_IMPLEMENTATION

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <curl/curl.h>

namespace llm {
namespace detail {

// RAII guard for curl_global_init/cleanup.
// Create ONE instance at program startup (e.g. first line of main).
// run_agent() does NOT call curl_global_init itself — this is intentional:
// calling it per-request is not thread-safe and violates libcurl's contract.
struct CurlGlobalGuard {
    CurlGlobalGuard()                                  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalGuard()                                 { curl_global_cleanup(); }
    CurlGlobalGuard(const CurlGlobalGuard&)            = delete;
    CurlGlobalGuard& operator=(const CurlGlobalGuard&) = delete;
};

struct CurlHandle {
    CURL* h;
    CurlHandle()                              : h(curl_easy_init()) {}
    ~CurlHandle()                             { if (h) curl_easy_cleanup(h); }
    CurlHandle(const CurlHandle&)             = delete;
    CurlHandle& operator=(const CurlHandle&) = delete;
};

struct SlistHandle {
    curl_slist* s = nullptr;
    ~SlistHandle()                              { if (s) curl_slist_free_all(s); }
    SlistHandle(const SlistHandle&)             = delete;
    SlistHandle& operator=(const SlistHandle&) = delete;
    SlistHandle()                               = default;
};

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* ud) {
    static_cast<std::string*>(ud)->append(ptr, size * nmemb);
    return size * nmemb;
}

static std::string post_json(const std::string& url,
                              const std::string& api_key,
                              const std::string& body,
                              long timeout_secs) {
    CurlHandle ch;
    if (!ch.h) throw std::runtime_error("curl_easy_init failed");
    SlistHandle sh;
    sh.s = curl_slist_append(sh.s, ("Authorization: Bearer " + api_key).c_str());
    sh.s = curl_slist_append(sh.s, "Content-Type: application/json");
    std::string resp;
    curl_easy_setopt(ch.h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(ch.h, CURLOPT_HTTPHEADER, sh.s);
    curl_easy_setopt(ch.h, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(ch.h, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(ch.h, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(ch.h, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(ch.h, CURLOPT_TIMEOUT, timeout_secs);
    CURLcode rc = curl_easy_perform(ch.h);
    if (rc != CURLE_OK) throw std::runtime_error(std::string("curl: ") + curl_easy_strerror(rc));
    return resp;
}

static std::string json_escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if      (c=='"')  out += "\\\"";
        else if (c=='\\') out += "\\\\";
        else if (c=='\n') out += "\\n";
        else if (c=='\r') out += "\\r";
        else if (c=='\t') out += "\\t";
        else if (c<0x20)  { out += "\\u00"; out += "0123456789abcdef"[c>>4]; out += "0123456789abcdef"[c&0xf]; }
        else              out += static_cast<char>(c);
    }
    return out;
}

static std::string json_str(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\":\"");
    if (pos == std::string::npos) return "";
    pos += key.size() + 4;
    std::string val;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) { ++pos; val += json[pos]; }
        else val += json[pos];
        ++pos;
    }
    return val;
}

static std::string json_str_raw(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return "";
    pos += key.size() + 3;
    while (pos < json.size() && json[pos] == ' ') ++pos;
    if (pos < json.size() && json[pos] == '"') {
        ++pos;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos]=='\\' && pos+1<json.size()){++pos;val+=json[pos];}
            else val+=json[pos];
            ++pos;
        }
        return val;
    }
    std::string val;
    while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != '\n')
        val += json[pos++];
    while (!val.empty() && (val.back()==' '||val.back()=='\r')) val.pop_back();
    return val;
}

// Recursively search nested JSON objects for a quoted string value.
// Handles models that nest the function name inside a "function":{...} block.
static std::string json_str_deep(const std::string& json, const std::string& key) {
    std::string direct = json_str(json, key);
    if (!direct.empty()) return direct;
    size_t depth = 0;
    for (size_t i = 0; i < json.size(); ++i) {
        if (json[i] == '{') {
            ++depth;
            if (depth > 1) {
                size_t d = 1, j = i + 1;
                while (j < json.size() && d > 0) {
                    if (json[j] == '{') ++d;
                    else if (json[j] == '}') --d;
                    ++j;
                }
                std::string r = json_str(json.substr(i, j - i), key);
                if (!r.empty()) return r;
                i = j - 1;
            }
        } else if (json[i] == '}' && depth > 0) {
            --depth;
        }
    }
    return "";
}

static std::string build_tools_json(const std::vector<Tool>& tools) {
    std::string out = "[";
    for (size_t ti = 0; ti < tools.size(); ++ti) {
        const auto& t = tools[ti];
        if (ti) out += ",";
        out += "{\"type\":\"function\",\"function\":{";
        out += "\"name\":\"" + json_escape(t.name) + "\",";
        out += "\"description\":\"" + json_escape(t.description) + "\",";
        out += "\"parameters\":{\"type\":\"object\",\"properties\":{";
        for (size_t pi = 0; pi < t.param_names.size(); ++pi) {
            if (pi) out += ",";
            std::string desc = pi < t.param_descriptions.size() ? t.param_descriptions[pi] : "";
            out += "\"" + json_escape(t.param_names[pi]) + "\":{\"type\":\"string\",\"description\":\"" + json_escape(desc) + "\"}";
        }
        out += "},\"required\":[";
        for (size_t pi = 0; pi < t.param_names.size(); ++pi) {
            if (pi) out += ",";
            out += "\"" + json_escape(t.param_names[pi]) + "\"";
        }
        out += "]}}}";
    }
    out += "]";
    return out;
}

static bool parse_tool_call(const std::string& resp,
                              std::string& tool_name,
                              std::string& args_json,
                              std::string& call_id) {
    auto pos = resp.find("\"tool_calls\"");
    if (pos == std::string::npos) return false;

    auto id_pos = resp.find("\"id\":", pos);
    if (id_pos != std::string::npos) call_id = json_str(resp.substr(id_pos), "id");

    auto fn_pos = resp.find("\"function\":", pos);
    if (fn_pos == std::string::npos) return false;
    tool_name = json_str_deep(resp.substr(fn_pos), "name");

    auto arg_pos = resp.find("\"arguments\":", pos);
    if (arg_pos == std::string::npos) return false;
    arg_pos += 12;
    while (arg_pos < resp.size() && resp[arg_pos]==' ') ++arg_pos;
    if (arg_pos < resp.size() && resp[arg_pos]=='"') {
        ++arg_pos;
        std::string raw;
        while (arg_pos < resp.size() && resp[arg_pos]!='"') {
            if (resp[arg_pos]=='\\' && arg_pos+1<resp.size()){++arg_pos;raw+=resp[arg_pos];}
            else raw+=resp[arg_pos];
            ++arg_pos;
        }
        args_json = raw;
    }
    return !tool_name.empty();
}

static std::map<std::string,std::string> parse_args(const std::string& args_json) {
    std::map<std::string,std::string> result;
    size_t i = 0;
    while (i < args_json.size()) {
        auto kstart = args_json.find('"', i);
        if (kstart == std::string::npos) break;
        ++kstart;
        auto kend = args_json.find('"', kstart);
        if (kend == std::string::npos) break;
        std::string key = args_json.substr(kstart, kend-kstart);
        auto colon = args_json.find(':', kend);
        if (colon == std::string::npos) break;
        ++colon;
        while (colon < args_json.size() && args_json[colon]==' ') ++colon;
        std::string val;
        if (colon < args_json.size() && args_json[colon]=='"') {
            ++colon;
            while (colon < args_json.size() && args_json[colon]!='"') {
                if (args_json[colon]=='\\' && colon+1<args_json.size()){++colon;val+=args_json[colon];}
                else val+=args_json[colon];
                ++colon;
            }
            ++colon;
        } else {
            auto vend = args_json.find_first_of(",}", colon);
            val = args_json.substr(colon, vend==std::string::npos ? std::string::npos : vend-colon);
            while (!val.empty() && val.back()==' ') val.pop_back();
            colon = vend;
        }
        result[key] = val;
        i = (colon == std::string::npos) ? args_json.size() : colon+1;
    }
    return result;
}

static std::string parse_content(const std::string& resp) {
    auto pos = resp.find("\"content\":");
    if (pos == std::string::npos) return "";
    pos += 10;
    while (pos < resp.size() && resp[pos]==' ') ++pos;
    if (pos < resp.size() && resp[pos]=='n') return "";
    if (pos < resp.size() && resp[pos]=='"') {
        ++pos;
        std::string val;
        while (pos < resp.size() && resp[pos]!='"') {
            if (resp[pos]=='\\' && pos+1<resp.size()){++pos;val+=resp[pos];}
            else val+=resp[pos];
            ++pos;
        }
        return val;
    }
    return "";
}

} // namespace detail

AgentResult run_agent(const std::string& prompt,
                       const std::vector<Tool>& tools,
                       const AgentConfig& config) {
    AgentResult result;
    result.iterations_used        = 0;
    result.max_iterations_reached = false;

    std::string messages = "[";
    if (!config.system_prompt.empty())
        messages += "{\"role\":\"system\",\"content\":\"" + detail::json_escape(config.system_prompt) + "\"},";
    messages += "{\"role\":\"user\",\"content\":\"" + detail::json_escape(prompt) + "\"}";

    std::string tools_json = detail::build_tools_json(tools);

    for (int iter = 0; iter < config.max_iterations; ++iter) {
        ++result.iterations_used;

        std::string body = "{\"model\":\"" + config.model + "\","
                           "\"messages\":" + messages + "],"
                           "\"tools\":" + tools_json + ","
                           "\"tool_choice\":\"auto\"}";

        if (config.verbose) std::cerr << "[agent] Iteration " << (iter+1) << " request...\n";

        std::string resp = detail::post_json(
            config.api_url, config.api_key, body, config.timeout_secs);

        if (config.verbose) std::cerr << "[agent] Response: " << resp.substr(0, 200) << "...\n";

        std::string finish = detail::json_str_raw(resp, "finish_reason");

        std::string tool_name, args_json, call_id;
        bool has_tool = detail::parse_tool_call(resp, tool_name, args_json, call_id);

        if (has_tool && finish != "stop") {
            auto args = detail::parse_args(args_json);

            std::string tool_result = "[tool not found]";
            for (const auto& t : tools) {
                if (t.name == tool_name) {
                    try { tool_result = t.fn(args); }
                    catch (const std::exception& e) { tool_result = std::string("[error] ") + e.what(); }
                    break;
                }
            }

            if (config.verbose) std::cerr << "[agent] Tool " << tool_name << " -> " << tool_result << "\n";

            AgentStep step;
            step.type        = AgentStep::Type::ToolCall;
            step.tool_name   = tool_name;
            step.tool_args   = args;
            step.tool_result = tool_result;
            result.steps.push_back(step);

            messages += ",{\"role\":\"assistant\",\"tool_calls\":[{\"id\":\"" +
                        detail::json_escape(call_id) + "\",\"type\":\"function\",\"function\":{\"name\":\"" +
                        detail::json_escape(tool_name) + "\",\"arguments\":\"" +
                        detail::json_escape(args_json) + "\"}}]}";
            messages += ",{\"role\":\"tool\",\"tool_call_id\":\"" +
                        detail::json_escape(call_id) + "\",\"content\":\"" +
                        detail::json_escape(tool_result) + "\"}";
        } else {
            std::string content = detail::parse_content(resp);
            result.answer = content;
            AgentStep step;
            step.type    = AgentStep::Type::FinalAnswer;
            step.content = content;
            result.steps.push_back(step);
            return result;
        }
    }

    result.max_iterations_reached = true;
    result.answer = "[max iterations reached]";
    return result;
}

} // namespace llm
#endif // LLM_AGENT_IMPLEMENTATION
