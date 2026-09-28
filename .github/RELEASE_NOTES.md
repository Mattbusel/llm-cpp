All 26 headers in one download: **llm-cpp-headers.zip**.

Inside:

- `include/`: the current `llm_*.hpp` from each of the 26 library repos
- `examples/offline/`: six programs that need no API key, with the output they print
- `README.txt`: the three-step setup and which headers need libcurl
- `LICENSE` (MIT)

Before this zip was built, CI compiled every implementation with g++ (C++17), linked all 26 into one binary with libcurl, and re-ran the offline examples against these exact headers, diffing their output.

Quick try, no key needed:

```bash
unzip llm-cpp-headers.zip && cd llm-cpp-headers
g++ -std=c++17 -Iinclude examples/offline/cache.cpp -o cache && ./cache
```

Browse what each header does: https://mattbusel.github.io/llm-cpp/
