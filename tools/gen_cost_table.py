#!/usr/bin/env python3
"""Regenerate the model price table in llm_cost.hpp from llm-prices.

llm-prices (https://github.com/Mattbusel/llm-prices) keeps one prices.json with
every model's standard rate, checked against each provider's pricing page.
This script rewrites the two generated regions of llm_cost.hpp from it:

    // BEGIN GENERATED PROVIDERS ... // END GENERATED PROVIDERS   (enum Provider)
    // BEGIN GENERATED MODELS ... // END GENERATED MODELS         (llm::models,
                                                                    lookup tables)

Everything outside those markers is hand-written and left alone.

Usage:
    python tools/gen_cost_table.py                    # rewrite include/llm/llm_cost.hpp
    python tools/gen_cost_table.py --check            # exit 1 if the header is out of date
    python tools/gen_cost_table.py --prices FILE|URL  # use another prices.json
    python tools/gen_cost_table.py --header PATH      # e.g. ../llm-cost/include/llm_cost.hpp

The default source is prices.json on llm-prices' main branch, so --check is
also the drift check: it fails as soon as llm-prices changes a price.
"""
import argparse
import difflib
import hashlib
import json
import pathlib
import re
import sys
import urllib.request
from decimal import Decimal

ROOT = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_PRICES = "https://raw.githubusercontent.com/Mattbusel/llm-prices/main/prices.json"
DEFAULT_HEADER = ROOT / "include" / "llm" / "llm_cost.hpp"

# llm-prices provider key -> enum name. OpenAI and Anthropic keep values 0 and 1.
PROVIDERS = [
    ("openai", "OpenAI"),
    ("anthropic", "Anthropic"),
    ("google", "Google"),
    ("deepseek", "DeepSeek"),
    ("mistral", "Mistral"),
    ("xai", "XAI"),
    ("cohere", "Cohere"),
    ("groq", "Groq"),
    ("together", "Together"),
    ("fireworks", "Fireworks"),
]
# Hosts serving other labs' open models get a provider prefix in the constant
# name, since the same id (e.g. openai/gpt-oss-120b) appears on several hosts.
HOSTS = {"groq", "together", "fireworks"}
STRIP_PREFIX = {"fireworks": "accounts/fireworks/models/"}

# The constants llm-cost shipped before the table was generated. Their names,
# Model::name strings and context windows stay as they were.
#   llm-prices id -> (constant, Model::name, context_window)
LEGACY = {
    "gpt-6-astra": ("GPT6_ASTRA", "gpt-6-astra", 1050000),
    "gpt-6-sol": ("GPT6_SOL", "gpt-6-sol", 1050000),
    "gpt-6-luna": ("GPT6_LUNA", "gpt-6-luna", 1050000),
    "gpt-4o": ("GPT4O", "gpt-4o", 128000),
    "gpt-4o-mini": ("GPT4O_MINI", "gpt-4o-mini", 128000),
    "gpt-4-turbo-2024-04-09": ("GPT4_TURBO", "gpt-4-turbo", 128000),
    "claude-fable-5-1": ("CLAUDE_FABLE_5_1", "claude-fable-5-1", 1000000),
    "claude-opus-5-5": ("CLAUDE_OPUS_5_5", "claude-opus-5-5", 1000000),
    "claude-sonnet-5": ("CLAUDE_SONNET_5", "claude-sonnet-5", 1000000),
    "claude-opus-4-5": ("CLAUDE_OPUS", "claude-opus-4-5", 200000),
    "claude-sonnet-4-5-20250929": ("CLAUDE_SONNET", "claude-sonnet-4-5", 200000),
    "claude-haiku-4-5": ("CLAUDE_HAIKU", "claude-haiku-4-5", 200000),
}


def load(src):
    if re.match(r"https?://", src):
        with urllib.request.urlopen(src, timeout=60) as r:
            raw = r.read()
    else:
        raw = pathlib.Path(src).read_bytes()
    return raw, json.loads(raw.decode("utf-8"))


def per_1k(v):
    if v is None:
        return "0.0"
    d = Decimal(str(v)) / 1000
    s = format(d.normalize(), "f")
    return s if "." in s else s + ".0"


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def const_name(provider, model_id):
    mid = model_id
    pre = STRIP_PREFIX.get(provider)
    if pre and mid.startswith(pre):
        mid = mid[len(pre):]
    name = re.sub(r"[^A-Za-z0-9]+", "_", mid).strip("_").upper()
    if provider in HOSTS:
        name = provider.upper() + "_" + name
    if name[0].isdigit():
        name = "M_" + name
    return name


def build(raw, data):
    enum = dict(PROVIDERS)
    unknown = sorted({m["provider"] for m in data["models"]} - set(enum))
    if unknown:
        sys.exit(f"gen_cost_table.py: add these providers to PROVIDERS: {unknown}")

    rows, seen = [], {}
    for m in data["models"]:
        legacy = LEGACY.get(m["model"])
        if legacy:
            const, name, ctx = legacy
        else:
            const, name, ctx = const_name(m["provider"], m["model"]), m["model"], 0
        if const in seen:
            sys.exit(f"gen_cost_table.py: {m['provider']}/{m['model']} and "
                     f"{seen[const]} both map to {const}")
        seen[const] = f"{m['provider']}/{m['model']}"
        aliases = list(m.get("aliases") or [])
        if name != m["model"]:
            aliases.insert(0, m["model"])
        rows.append(dict(const=const, name=name, ctx=ctx, provider=enum[m["provider"]],
                         input=per_1k(m["input"]), output=per_1k(m["output"]),
                         retired=m["status"] != "listed", aliases=aliases,
                         embedding=m["output"] is None))
    missing = sorted(set(LEGACY) - {m["model"] for m in data["models"]})
    if missing:
        sys.exit(f"gen_cost_table.py: llm-prices no longer has {missing}; "
                 "keep their constants by hand or drop them in a major release")

    sha = hashlib.sha256(raw).hexdigest()
    providers = "".join(f"    {e},\n" for _, e in PROVIDERS)
    prov_block = f"enum class Provider {{\n{providers}}};\n"

    listed = [r for r in rows if not r["retired"]]
    retired = [r for r in rows if r["retired"]]
    w = min(max(len(r["const"]) for r in rows), 24)
    out = []
    out.append(f"// Source: llm-prices prices.json, generated {data['generated']},")
    out.append(f"// sha256 {sha[:16]}. {len(listed)} listed models, {len(retired)} retired.")
    out.append("// context_window is 0 (not known) except for the models llm-cost shipped")
    out.append("// before the table was generated.")
    out.append("namespace models {")
    current = None
    for r in listed + retired:
        group = ("retired: no longer on the provider's pricing page; prices as last listed"
                 if r["retired"] else r["provider"])
        if group != current:
            out.append(f"    // {group}")
            current = group
        out.append(f"    inline const Model {r['const'].ljust(w)} = {{{cstr(r['name'])}, "
                   f"Provider::{r['provider']}, {r['input']}, {r['output']}, {r['ctx']}}};")
    out.append("} // namespace models")
    out.append("")
    out.append("namespace detail {")
    out.append("struct ModelAlias { const char* alias; const Model* model; };")
    out.append("inline const Model* const listed_models[] = {")
    out += [f"    &models::{r['const']}," for r in listed]
    out.append("};")
    out.append("inline const Model* const retired_models[] = {")
    out += [f"    &models::{r['const']}," for r in retired]
    out.append("};")
    out.append("inline const ModelAlias model_aliases[] = {")
    for r in rows:
        for a in r["aliases"]:
            out.append(f"    {{{cstr(a)}, &models::{r['const']}}},")
    out.append("};")
    out.append("} // namespace detail")
    return prov_block, "\n".join(out) + "\n", len(listed)


def splice(text, tag, body):
    pat = re.compile(r"(// BEGIN GENERATED " + tag + r"[^\n]*\n)(.*?)(// END GENERATED " + tag + r")",
                     re.S)
    if not pat.search(text):
        sys.exit(f"gen_cost_table.py: markers for {tag} not found")
    return pat.sub(lambda m: m.group(1) + body + m.group(3), text, count=1)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--prices", default=DEFAULT_PRICES, help="prices.json path or URL")
    ap.add_argument("--header", default=str(DEFAULT_HEADER), help="llm_cost.hpp to rewrite")
    ap.add_argument("--check", action="store_true", help="do not write; exit 1 on drift")
    args = ap.parse_args()

    raw, data = load(args.prices)
    prov, models, n = build(raw, data)
    path = pathlib.Path(args.header)
    old = path.read_text("utf-8")
    new = splice(splice(old, "PROVIDERS", prov), "MODELS", models)
    if args.check:
        if new != old:
            sys.stdout.writelines(difflib.unified_diff(
                old.splitlines(True), new.splitlines(True), str(path), "regenerated", n=1))
            print(f"\n{path} is out of date with {args.prices}.\n"
                  "Run python tools/gen_cost_table.py, commit the header to "
                  "Mattbusel/llm-cost too, and update examples/offline/output/cost.txt.")
            return 1
        print(f"{path}: up to date ({n} listed models)")
        return 0
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(new)
    print(f"{path}: {n} listed models from {data['generated']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
