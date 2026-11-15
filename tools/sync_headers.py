#!/usr/bin/env python3
"""Vendor the 26 single-header libraries into include/llm/.

For each library in tools/libraries.json this copies
    <repo>/include/llm_<name>.hpp   -> include/llm/llm_<name>.hpp
    <repo>/examples/*.cpp           -> tests/examples/<name>/*.cpp
    <repo>/tests/*.cpp              -> tests/upstream/<name>/*.cpp
byte for byte, and rewrites include/llm/SOURCES.md with the source repo,
commit and SHA-256 of every header.

Usage:
    python tools/sync_headers.py                 # shallow-clone each repo's main branch
    python tools/sync_headers.py --src DIR       # use existing clones in DIR/llm-<name>
"""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
OWNER = "Mattbusel"


def git(*args, cwd=None):
    return subprocess.run(["git", *args], cwd=cwd, check=True,
                          capture_output=True, text=True).stdout.strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", help="directory holding llm-<name> clones")
    ap.add_argument("--ref", default="main", help="branch to clone (default main)")
    args = ap.parse_args()

    libs = json.loads((ROOT / "tools" / "libraries.json").read_text("utf-8"))["libraries"]
    tmp = None
    src = pathlib.Path(args.src) if args.src else None
    if src is None:
        tmp = tempfile.TemporaryDirectory()
        src = pathlib.Path(tmp.name)

    inc = ROOT / "include" / "llm"
    inc.mkdir(parents=True, exist_ok=True)
    rows = []
    for lib in sorted(libs, key=lambda l: l["name"]):
        repo = lib["name"]
        short = repo[len("llm-"):].replace("-", "_")
        clone = src / repo
        if not clone.exists():
            git("clone", "-q", "--depth", "1", "--branch", args.ref,
                "-c", "core.autocrlf=false",
                f"https://github.com/{OWNER}/{repo}.git", str(clone))
        sha = git("rev-parse", "HEAD", cwd=clone)
        date = git("log", "-1", "--format=%cs", cwd=clone)
        header = clone / "include" / f"llm_{short}.hpp"
        if not header.exists():
            sys.exit(f"{repo}: missing {header}")
        data = header.read_bytes()
        (inc / header.name).write_bytes(data)

        for sub, dest in (("examples", "examples"), ("tests", "upstream")):
            out = ROOT / "tests" / dest / short
            if out.exists():
                shutil.rmtree(out)
            files = sorted((clone / sub).glob("*.cpp")) if (clone / sub).is_dir() else []
            if files:
                out.mkdir(parents=True)
                for f in files:
                    (out / f.name).write_bytes(f.read_bytes())

        lic = (clone / "LICENSE").read_text("utf-8", "replace") if (clone / "LICENSE").exists() else ""
        license_id = "MIT" if "MIT License" in lic or "Permission is hereby granted" in lic else "see repo"
        rows.append((header.name, repo, sha, date, license_id,
                     lib["deps"], lib["macro"], hashlib.sha256(data).hexdigest()))

    lines = [
        "# Sources",
        "",
        "Every header in this directory is a byte-for-byte copy of `include/<file>` from the",
        "repository and commit listed below. Regenerate with `python tools/sync_headers.py`.",
        "",
        "| Header | Source repository | Commit | Date | License | Needs | Implementation macro | SHA-256 |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for name, repo, sha, date, lic, deps, macro, digest in rows:
        needs = "libcurl" if deps == "curl" else "none"
        lines.append(
            f"| `{name}` | [{OWNER}/{repo}](https://github.com/{OWNER}/{repo}) "
            f"| [`{sha[:12]}`](https://github.com/{OWNER}/{repo}/tree/{sha}) | {date} | {lic} "
            f"| {needs} | `{macro}` | `{digest[:16]}` |")
    lines += [
        "",
        "Full commit hashes:",
        "",
        "```",
        *[f"{repo:<14} {sha}" for _, repo, sha, *_ in rows],
        "```",
        "",
    ]
    (inc / "SOURCES.md").write_text("\n".join(lines), "utf-8", newline="\n")
    print(f"vendored {len(rows)} headers into {inc}")
    if tmp:
        tmp.cleanup()


if __name__ == "__main__":
    main()
