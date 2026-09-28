# Contributing to llm-cpp

llm-cpp is the index for 26 single-header C++17 libraries. Each library has its own repository, `github.com/Mattbusel/llm-<name>`, with the header in `include/llm_<name>.hpp`.

## Where to send what

- **A bug or improvement in one library**: open the issue or pull request in that library's repo.
- **This repo** holds the README, the catalogue site (`docs/`, built by `python tools/build_site.py` from `tools/libraries.json` and `tools/site_template.html`), the offline examples in `examples/offline/`, and the release workflow that zips all 26 headers.

## Rules for the headers

- C++17, single file, stb-style: declarations always, implementation only under `#ifdef LLM_<NAME>_IMPLEMENTATION`.
- Offline libraries use only the standard library. Network libraries may use libcurl and nothing else.

## Checking a change here

```bash
python tools/build_site.py          # regenerates docs/index.html; CI fails if it drifts
g++ -std=c++17 -Ithird_party examples/offline/cache.cpp -o cache && ./cache
```

If an example's output changes, update the matching file in `examples/offline/output/`; CI diffs them.
