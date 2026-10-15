<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/banner-dark.png">
  <img alt="llm-cpp: llm_cache.hpp 헤더와 그 옆의 터미널. 터미널은 헤더를 내려받고 MSVC로 예제를 컴파일한 뒤 실제 캐시 적중과 축출을 출력한다" src="assets/banner-light.png">
</picture>

# llm-cpp: LLM 기능을 위한 싱글 헤더 C++ 라이브러리

[English](README.md) | [简体中文](README.zh-CN.md) | [日本語](README.ja.md) | 한국어

**파일 하나만 복사하면 C++ 프로그램에 ChatGPT나 Claude 기능을 넣을 수 있습니다.** 스트리밍, 재시도, 캐싱, 비용 추정, RAG, 구조화된 JSON 출력, 도구 호출 에이전트 외 19가지 기능을 OpenAI와 Anthropic API용 싱글 헤더 C++17 라이브러리 26개로 제공합니다. SDK도 패키지 매니저도 필요 없습니다.

**대상**: 게임, 데스크톱 앱, 트레이딩 시스템, 임베디드 도구, 서비스를 만들면서 Python 사이드카 없이 LLM을 호출하고 싶은 C++ 개발자.

[![Latest release](https://img.shields.io/github/v/release/Mattbusel/llm-cpp)](https://gitlab.com/mattbusel/llm-cpp/-/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

## 다운로드

### [헤더 26개 전체 다운로드 (.zip)](https://gitlab.com/mattbusel/llm-cpp/-/releases/permalink/latest/downloads/llm-cpp-headers.zip)

zip에는 모든 `llm_*.hpp`가 들어 있는 `include/`, 오프라인 예제 프로그램 6개, 짧은 README가 있습니다. 필요한 것 하나만 받아도 됩니다:

```bash
curl -fsSLO https://gitlab.com/mattbusel/llm-cache/-/raw/main/include/llm_cache.hpp
```

**[카탈로그 사이트 둘러보기](https://llm-cpp.vercel.app/)**: 라이브러리 26개를 필터링하고, 실제 출력을 보고, 고른 헤더에 맞춘 설치 명령을 받을 수 있습니다.

## 어떤 헤더가 필요할까?

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/img/picker-dark.svg">
  <img alt="작업 16가지를 헤더에 대응시킨 차트. 답변 스트리밍: llm_stream. 재시도와 페일오버: llm_retry. 반복 프롬프트 건너뛰기: llm_cache. 프롬프트 비용 계산: llm_cost. 유효한 JSON: llm_format과 llm_json. 기억하는 챗봇: llm_chat과 llm_retry. 문서 질의응답: llm_rag와 llm_rank. 긴 대화: llm_compress. 도구 호출: llm_agent. 개인정보와 키 제거: llm_guard. 더 저렴한 모델로 라우팅: llm_router. 로깅: llm_log와 llm_trace. 프롬프트 일괄 처리: llm_batch. 네트워크 없는 테스트: llm_mock. 이미지: llm_vision. 오디오: llm_audio. 초록색 헤더는 오프라인용이고, 주황색 헤더는 libcurl이 필요하다." src="docs/img/picker-light.svg" width="830">
</picture>

초록색 헤더는 C++ 표준 라이브러리만 씁니다. 주황색 헤더는 OpenAI나 Anthropic API를 호출하므로 libcurl이 필요합니다. 26개 전체와 각각의 한 줄 설명: [docs/REFERENCE.md](docs/REFERENCE.md).

## 동작 원리

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/img/how-it-works-dark.svg">
  <img alt="네 단계. 1: curl로 llm_cache.hpp를 프로젝트에 받는다. 2: 헤더의 1~85행은 선언이고, 86~210행은 #ifdef LLM_CACHE_IMPLEMENTATION 뒤에 있는 구현이다. 3: cache.cpp는 include하기 전에 LLM_CACHE_IMPLEMENTATION을 정의해 구현 코드를 가져오고, 다른 파일은 include만 한다. 4: cl로 cache.cpp를 컴파일하면 프로그램이 캐시 적중 1번, 미스 4번, 축출 2번을 출력한다." src="docs/img/how-it-works-light.svg" width="830">
</picture>

모든 헤더가 이 stb 스타일 패턴을 따르므로, 하나를 써 봤다면 26개를 모두 써 본 셈입니다. 여러 개를 함께 쓰나요? 구현마다 별도의 `.cpp`를 두세요: [docs/USING-SEVERAL.md](docs/USING-SEVERAL.md).

## 예제(실제 출력, API 키 불필요)

[`examples/offline`](examples/offline)에 있는 실제 프로그램입니다. CI는 푸시할 때마다 각 라이브러리의 현재 헤더로 g++를 사용해 이 프로그램들을 다시 빌드하고, 아래 출력이 바뀌면 실패합니다.

**프롬프트가 앱 밖으로 나가기 전에 개인정보와 API 키 제거하기**([guard.cpp](examples/offline/guard.cpp), llm-guard):

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/example-guard-dark.png">
  <img alt="guard.cpp가 프롬프트를 검사해 이메일, 카드 번호, API 키를 찾아내고, 인젝션 점수를 0.75로 매긴 뒤 정리된 텍스트를 출력한다" src="assets/example-guard-light.png" width="830">
</picture>

**프롬프트 하나의 비용을 모델별로 계산하고, 1센트를 넘을 호출은 거부하기**([cost.cpp](examples/offline/cost.cpp), llm-cost):

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

**모델이 따를 때까지 다시 프롬프트해서 스키마에 맞는 JSON 받기**([format.cpp](examples/offline/format.cpp), llm-format; 대역 람다가 모델 역할을 함):

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

폴더에는 [cache.cpp](examples/offline/cache.cpp)(위 다이어그램의 출력), [json.cpp](examples/offline/json.cpp), [compress.cpp](examples/offline/compress.cpp)도 있습니다. 2026-09-28에 MSVC 19.44로 컴파일했으며, llm-cost의 토큰 수는 근삿값입니다.

## 3단계로 사용하기

1. 헤더를 프로젝트에 **복사합니다**(zip에서 꺼내거나 위처럼 `curl -fsSLO`로).
2. 딱 하나의 `.cpp`에서 **구현 코드를 켭니다**:
   ```cpp
   #define LLM_CACHE_IMPLEMENTATION
   #include "llm_cache.hpp"
   ```
   다른 파일은 `#include "llm_cache.hpp"`만 쓰면 됩니다.
3. **C++17로 컴파일합니다**: `g++ -std=c++17 main.cpp` 또는 `cl /std:c++17 /EHsc main.cpp`. libcurl 표시가 있는 헤더는 `-lcurl`도 필요합니다(macOS에는 기본 설치, `apt install libcurl4-openssl-dev`, `vcpkg install curl`).

## 문서

| | |
|---|---|
| [카탈로그 사이트](https://llm-cpp.vercel.app/) | 26개 전체 필터링, 실제 출력, 자동 생성되는 설치 명령 |
| [docs/REFERENCE.md](docs/REFERENCE.md) | 각 라이브러리가 하는 일과 필요한 것, 요구 사항, 상태 |
| [docs/USING-SEVERAL.md](docs/USING-SEVERAL.md) | 헤더 조합하기: 스트리밍 + 재시도 + 로깅 프로그램, 오프라인 RAG 파이프라인 |
| [examples/offline](examples/offline) | API 키가 필요 없는 프로그램 6개와 커밋된 출력 |
| [Releases](https://gitlab.com/mattbusel/llm-cpp/-/releases) | CI가 빌드하고 링크를 검사한 헤더 zip |

각 라이브러리는 별도의 저장소(`gitlab.com/mattbusel/llm-<name>`)에 있으며, 이슈와 풀 리퀘스트는 그곳에서 환영합니다.

## 작성자에게 의뢰하기

**여러분의 제품에도 이런 엔지니어링이 필요한가요?** 소수의 클라이언트 프로젝트를 맡고 있습니다: LLM 기능, iOS 앱, 성능 개선을 고정 가격으로. [서비스와 가격](https://tensorust.com/) · [이메일](mailto:mattbusel@gmail.com) · [LinkedIn](https://www.linkedin.com/in/matthewbusel/)
