<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/banner-dark.png">
  <img alt="llm-cpp：llm_cache.hpp 头文件，旁边的终端下载了它，用 MSVC 编译一个示例，并打印出真实的缓存命中和淘汰" src="assets/banner-light.png">
</picture>

# llm-cpp：为大模型（LLM）功能打造的单头文件 C++ 库

[English](README.md) | 简体中文 | [日本語](README.ja.md) | [한국어](README.ko.md)

**复制一个文件，就能给 C++ 程序加上 ChatGPT 或 Claude 的功能**。流式输出、重试、缓存、费用估算、RAG、结构化 JSON 输出、工具调用 Agent 以及另外 19 项功能，以 26 个面向 OpenAI 和 Anthropic API 的单头文件 C++17 库提供。不需要 SDK，不需要包管理器。

**适合谁**：正在开发游戏、桌面应用、交易系统、嵌入式工具或后端服务，想直接调用 LLM 而不想额外挂一个 Python 进程的 C++ 开发者。

[![Latest release](https://img.shields.io/github/v/release/Mattbusel/llm-cpp)](https://gitlab.com/mattbusel/llm-cpp/-/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

## 下载

### [下载全部 26 个头文件 (.zip)](https://gitlab.com/mattbusel/llm-cpp/-/releases/permalink/latest/downloads/llm-cpp-headers.zip)

压缩包里有包含所有 `llm_*.hpp` 的 `include/` 目录、六个离线示例程序和一份简短的 README。也可以只拿你需要的那一个：

```bash
curl -fsSLO https://gitlab.com/mattbusel/llm-cache/-/raw/main/include/llm_cache.hpp
```

**[浏览目录网站](https://llm-cpp.vercel.app/)**：筛选这 26 个库，查看真实输出，并为你选中的头文件生成安装命令。

## 我需要哪个头文件？

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/img/picker-dark.svg">
  <img alt="一张把十六项任务对应到头文件的图。流式输出回复：llm_stream。重试与故障切换：llm_retry。跳过重复提示词：llm_cache。估算提示词费用：llm_cost。合法 JSON：llm_format 加 llm_json。带记忆的聊天机器人：llm_chat 加 llm_retry。文档问答：llm_rag 加 llm_rank。长对话：llm_compress。工具调用：llm_agent。清除个人信息和密钥：llm_guard。路由到更便宜的模型：llm_router。日志：llm_log 加 llm_trace。批量提示词：llm_batch。无网络测试：llm_mock。图像：llm_vision。音频：llm_audio。绿色头文件可离线使用；橙色头文件需要 libcurl。" src="docs/img/picker-light.svg" width="830">
</picture>

绿色头文件只用 C++ 标准库。橙色头文件会调用 OpenAI 或 Anthropic API，需要 libcurl。全部 26 个，各附一行说明：[docs/REFERENCE.md](docs/REFERENCE.md)。

## 工作原理

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/img/how-it-works-dark.svg">
  <img alt="四个步骤。1：用 curl 把 llm_cache.hpp 下载到你的项目里。2：头文件第 1 到 85 行是声明，第 86 到 210 行是放在 #ifdef LLM_CACHE_IMPLEMENTATION 后面的实现。3：cache.cpp 在 include 之前定义 LLM_CACHE_IMPLEMENTATION，从而得到实现代码；其他文件只需 include 它。4：cl 编译 cache.cpp，程序打印出一次缓存命中、四次未命中和两次淘汰。" src="docs/img/how-it-works-light.svg" width="830">
</picture>

每个头文件都遵循这种 stb 风格的模式，所以用过一个，就等于 26 个都会用了。要同时用好几个？给每个实现单独一个 `.cpp`：[docs/USING-SEVERAL.md](docs/USING-SEVERAL.md)。

## 示例（真实输出，无需 API 密钥）

这些都是 [`examples/offline`](examples/offline) 里的真实程序。每次推送时，CI 都会用 g++ 针对每个库当前的头文件重新编译它们，只要下面的输出有变化就会失败。

**在提示词离开你的应用之前，清除个人数据和 API 密钥**（[guard.cpp](examples/offline/guard.cpp)，llm-guard）：

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/example-guard-dark.png">
  <img alt="guard.cpp 扫描一段提示词，发现一个邮箱、一个卡号和一个 API 密钥，给出 0.75 的注入评分，并打印清理后的文本" src="assets/example-guard-light.png" width="830">
</picture>

**估算同一个提示词在各个模型上的费用，并拒绝花费会超过一美分的调用**（[cost.cpp](examples/offline/cost.cpp)，llm-cost）：

```text
C:\demo> cl /nologo /std:c++17 /EHsc cost.cpp && cost.exe
cost.cpp
gpt-6-luna          4080 tokens  0.0408¢
gpt-4o-mini         4080 tokens  0.0612¢
claude-haiku-4-5    4080 tokens  0.4080¢
gpt-6-sol           4080 tokens  0.8160¢
claude-sonnet-5     4080 tokens  0.8160¢
gpt-4o              4080 tokens  $0.0102
claude-sonnet-4-5   4080 tokens  $0.0122
claude-opus-5-5     4080 tokens  $0.0163
claude-opus-4-5     4080 tokens  $0.0204
gpt-6-astra         4080 tokens  $0.0408
gpt-4-turbo         4080 tokens  $0.0408
claude-fable-5-1    4080 tokens  $0.0408

blocked: Budget exceeded: estimated $0.0204 > limit $0.0100 (4080 tokens on claude-opus-4-5)
```

**拿到符合 schema 的 JSON，不合格就重新提示，直到模型照做**（[format.cpp](examples/offline/format.cpp)，llm-format；由一个替身 lambda 扮演模型）：

```text
valid: yes after 2 attempt(s)
{
  "priority": 1,
  "tags": [
    "auth"
  ],
  "title": "Login fails"
}
error: Field "title" has wrong type: expected string
error: Missing required field: "priority"
error: Missing required field: "tags"
```

这个目录里还有：[cache.cpp](examples/offline/cache.cpp)（就是上面示意图里的输出）、[json.cpp](examples/offline/json.cpp) 和 [compress.cpp](examples/offline/compress.cpp)。于 2026-09-28 用 MSVC 19.44 编译；llm-cost 中的 token 数是近似值。

## 3 步上手

1. **复制**头文件到你的项目里（从压缩包里拿，或者像上面那样用 `curl -fsSLO`）。
2. **启用实现代码**，只在一个 `.cpp` 里这样写：
   ```cpp
   #define LLM_CACHE_IMPLEMENTATION
   #include "llm_cache.hpp"
   ```
   其他文件只需写 `#include "llm_cache.hpp"`。
3. **按 C++17 编译**：`g++ -std=c++17 main.cpp` 或 `cl /std:c++17 /EHsc main.cpp`。标注了 libcurl 的头文件还需要 `-lcurl`（macOS 已预装，`apt install libcurl4-openssl-dev`，`vcpkg install curl`）。

## 文档

| | |
|---|---|
| [目录网站](https://llm-cpp.vercel.app/) | 筛选全部 26 个库、真实输出、自动生成的安装命令 |
| [docs/REFERENCE.md](docs/REFERENCE.md) | 每个库的功能和依赖、环境要求、状态 |
| [docs/USING-SEVERAL.md](docs/USING-SEVERAL.md) | 组合使用多个头文件：一个流式输出 + 重试 + 日志的程序、一条离线 RAG 流水线 |
| [examples/offline](examples/offline) | 六个无需 API 密钥的程序，附带已提交的输出 |
| [Releases](https://gitlab.com/mattbusel/llm-cpp/-/releases) | 头文件压缩包，由 CI 构建并检查链接 |

每个库都有自己的仓库（`gitlab.com/mattbusel/llm-<name>`）；欢迎在那里提交 issue 和 pull request。

## 雇用作者

**你的产品需要这类工程能力吗？** 我会接少量客户项目：LLM 功能、iOS 应用和性能优化，固定价格。[服务与报价](https://mattbusel.vercel.app/) · [邮件](mailto:mattbusel@gmail.com) · [LinkedIn](https://www.linkedin.com/in/matthewbusel/)
