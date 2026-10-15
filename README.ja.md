<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/banner-dark.png">
  <img alt="llm-cpp：llm_cache.hpp ヘッダーと、その横でそれをダウンロードし、MSVC でサンプルをコンパイルして、実際のキャッシュヒットと追い出しを表示するターミナル" src="assets/banner-light.png">
</picture>

# llm-cpp：LLM 機能のためのシングルヘッダー C++ ライブラリ

[English](README.md) | [简体中文](README.zh-CN.md) | 日本語 | [한국어](README.ko.md)

**ファイルを 1 つコピーするだけで、C++ プログラムに ChatGPT や Claude の機能を追加できます**。ストリーミング、リトライ、キャッシュ、コスト見積もり、RAG、構造化 JSON 出力、ツール呼び出しエージェント、ほか 19 の機能を、OpenAI と Anthropic の API 向けの 26 個のシングルヘッダー C++17 ライブラリとして提供します。SDK もパッケージマネージャーも不要です。

**対象**：ゲーム、デスクトップアプリ、トレーディングシステム、組み込みツール、サービスを開発していて、Python のサイドカーなしで LLM を呼び出したい C++ 開発者。

[![Latest release](https://img.shields.io/github/v/release/Mattbusel/llm-cpp)](https://gitlab.com/mattbusel/llm-cpp/-/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

## ダウンロード

### [26 個のヘッダーをまとめてダウンロード (.zip)](https://gitlab.com/mattbusel/llm-cpp/-/releases/permalink/latest/downloads/llm-cpp-headers.zip)

zip には、すべての `llm_*.hpp` を収めた `include/`、オフラインで動く 6 つのサンプルプログラム、短い README が入っています。必要なものだけを取ってくることもできます。

```bash
curl -fsSLO https://gitlab.com/mattbusel/llm-cache/-/raw/main/include/llm_cache.hpp
```

**[カタログサイトを見る](https://llm-cpp.vercel.app/)**：26 個のライブラリを絞り込み、実際の出力を確認し、選んだヘッダー用のインストールコマンドを生成できます。

## どのヘッダーが必要？

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/img/picker-dark.svg">
  <img alt="16 個のタスクをヘッダーに対応づけた図。返答をストリーミング：llm_stream。リトライとフェイルオーバー：llm_retry。同じプロンプトを省略：llm_cache。プロンプトの料金計算：llm_cost。正しい JSON：llm_format と llm_json。記憶を持つチャットボット：llm_chat と llm_retry。ドキュメントへの質疑応答：llm_rag と llm_rank。長い会話：llm_compress。ツール呼び出し：llm_agent。個人情報とキーの除去：llm_guard。安いモデルへのルーティング：llm_router。ログ：llm_log と llm_trace。プロンプトの一括処理：llm_batch。ネットワークなしのテスト：llm_mock。画像：llm_vision。音声：llm_audio。緑のヘッダーはオフラインで動き、オレンジのヘッダーは libcurl が必要。" src="docs/img/picker-light.svg" width="830">
</picture>

緑のヘッダーは C++ 標準ライブラリだけを使います。オレンジのヘッダーは OpenAI か Anthropic の API を呼び出すので、libcurl が必要です。全 26 個とそれぞれの 1 行説明：[docs/REFERENCE.md](docs/REFERENCE.md)。

## 仕組み

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/img/how-it-works-dark.svg">
  <img alt="4 つのステップ。1：llm_cache.hpp を curl でプロジェクトに取ってくる。2：ヘッダーの 1 行目から 85 行目は宣言で、86 行目から 210 行目は #ifdef LLM_CACHE_IMPLEMENTATION の内側にある実装。3：cache.cpp はインクルードの前に LLM_CACHE_IMPLEMENTATION を定義して実装を取り込み、ほかのファイルは単にインクルードするだけ。4：cl で cache.cpp をコンパイルすると、プログラムがキャッシュヒット 1 回、ミス 4 回、追い出し 2 回を表示する。" src="docs/img/how-it-works-light.svg" width="830">
</picture>

どのヘッダーもこの stb スタイルのパターンに従っているので、1 つ使えば 26 個すべての使い方がわかります。複数を同時に使う場合は、実装ごとに専用の `.cpp` を用意してください：[docs/USING-SEVERAL.md](docs/USING-SEVERAL.md)。

## サンプル（実際の出力、API キー不要）

[`examples/offline`](examples/offline) にある実際のプログラムです。CI はプッシュのたびに、各ライブラリの最新のヘッダーに対してこれらを g++ でビルドし直し、下の出力が変わったら失敗します。

**プロンプトがアプリの外に出る前に、個人情報と API キーを取り除く**（[guard.cpp](examples/offline/guard.cpp)、llm-guard）：

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/example-guard-dark.png">
  <img alt="guard.cpp がプロンプトをスキャンしてメールアドレス、カード番号、API キーを見つけ、インジェクションのスコアを 0.75 とし、除去後のテキストを表示する" src="assets/example-guard-light.png" width="830">
</picture>

**1 つのプロンプトの料金をモデルごとに計算し、1 セントを超える呼び出しを拒否する**（[cost.cpp](examples/offline/cost.cpp)、llm-cost）：

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

**スキーマに合った JSON を、モデルが従うまでプロンプトを出し直して手に入れる**（[format.cpp](examples/offline/format.cpp)、llm-format。モデル役は代わりのラムダが務めます）：

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

フォルダーにはほかに [cache.cpp](examples/offline/cache.cpp)（上の図の出力）、[json.cpp](examples/offline/json.cpp)、[compress.cpp](examples/offline/compress.cpp) もあります。2026-09-28 に MSVC 19.44 でコンパイルしました。llm-cost のトークン数は概算です。

## 3 ステップで使う

1. ヘッダーをプロジェクトに**コピーする**（zip から、または上の `curl -fsSLO` で）。
2. ちょうど 1 つの `.cpp` で**実装を有効にする**：
   ```cpp
   #define LLM_CACHE_IMPLEMENTATION
   #include "llm_cache.hpp"
   ```
   ほかのファイルは `#include "llm_cache.hpp"` と書くだけです。
3. **C++17 としてコンパイルする**：`g++ -std=c++17 main.cpp` または `cl /std:c++17 /EHsc main.cpp`。libcurl と書かれたヘッダーには `-lcurl` も必要です（macOS にはプリインストール済み、`apt install libcurl4-openssl-dev`、`vcpkg install curl`）。

## ドキュメント

| | |
|---|---|
| [カタログサイト](https://llm-cpp.vercel.app/) | 26 個すべてを絞り込み、実際の出力、生成されるインストールコマンド |
| [docs/REFERENCE.md](docs/REFERENCE.md) | 各ライブラリの機能と必要なもの、動作要件、ステータス |
| [docs/USING-SEVERAL.md](docs/USING-SEVERAL.md) | ヘッダーの組み合わせ方：ストリーミング + リトライ + ログのプログラム、オフラインの RAG パイプライン |
| [examples/offline](examples/offline) | API キー不要の 6 つのプログラムと、コミット済みの出力 |
| [Releases](https://gitlab.com/mattbusel/llm-cpp/-/releases) | CI がビルドしてリンクをチェックしたヘッダーの zip |

各ライブラリはそれぞれ専用のリポジトリ（`gitlab.com/mattbusel/llm-<name>`）にあります。Issue やプルリクエストはそちらで歓迎しています。

## 作者に依頼する

**あなたのプロダクトにもこうしたエンジニアリングが必要ですか？** 少数のクライアント案件をお受けしています：LLM 機能、iOS アプリ、パフォーマンス改善を固定価格で。[サービスと料金](https://tensorust.com/) · [メール](mailto:mattbusel@gmail.com) · [LinkedIn](https://www.linkedin.com/in/matthewbusel/)
