// json_types.cpp: demonstrate all JSON value types and round-trip parsing
#define LLM_JSON_IMPLEMENTATION
#include "llm_json.hpp"
#include <cstdio>
#include <cassert>

int main() {
    // Test all types
    llm::json::Value null_val;
    llm::json::Value bool_val(true);
    llm::json::Value int_val(42LL);
    llm::json::Value float_val(3.14);
    llm::json::Value str_val("hello \"world\"");
    llm::json::Value arr_val = {1, 2, 3};

    std::printf("null:  %s\n", llm::json::dump(null_val).c_str());
    std::printf("bool:  %s\n", llm::json::dump(bool_val).c_str());
    std::printf("int:   %s\n", llm::json::dump(int_val).c_str());
    std::printf("float: %s\n", llm::json::dump(float_val).c_str());
    std::printf("str:   %s\n", llm::json::dump(str_val).c_str());
    std::printf("arr:   %s\n", llm::json::dump(arr_val).c_str());

    // Round-trip
    std::string serialized = llm::json::dump(arr_val);
    auto parsed = llm::json::parse(serialized);
    assert(parsed.size() == 3);
    assert(parsed[0].as_int() == 1);
    std::printf("\nRound-trip OK: %s\n", serialized.c_str());
    return 0;
}
