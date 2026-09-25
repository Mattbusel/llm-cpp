"""Build docs/index.html for the llm-cpp GitHub Pages site.

Inputs (all in this repo):
  tools/libraries.json          catalogue data: the README tables plus line counts
  examples/offline/<lib>.cpp    example programs shown on the page
  examples/offline/output/*.txt their captured output (MSVC 19.44, x64)

Run:  python tools/build_site.py
"""
import html
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = json.load(open(os.path.join(ROOT, "tools", "libraries.json"), encoding="utf-8"))
LIBS = DATA["libraries"]
RAW = "https://raw.githubusercontent.com/Mattbusel/{name}/main/include/{file}"
REPO = "https://github.com/Mattbusel/{name}"
BUILD_DATE = "2026-09-25"
COMPILER = "MSVC 19.44 x64"

EXAMPLES = [
    ("cache", "llm-cache", "Identical prompts skip the API. Keys are case-insensitive by default, and the least recently used entry is evicted at capacity."),
    ("cost", "llm-cost", "Price a prompt across the built-in model table before you send it, and refuse calls over a budget."),
    ("guard", "llm-guard", "Find and scrub emails, card numbers and API keys, and score a prompt against known injection phrases."),
    ("format", "llm-format", "Validate model JSON against a schema and re-prompt until it conforms. A stand-in lambda plays the model here."),
    ("json", "llm-json", "Build request bodies and read responses without pulling in a JSON library."),
    ("compress", "llm-compress", "Keep a long chat inside a token budget. The pinned system prompt always survives."),
]

CAT_LABEL = {"core": "Core", "data": "Data and retrieval", "ops": "Operations and testing", "app": "Application features"}

KEYWORDS = set("""alignas alignof auto bool break case catch char class const constexpr continue decltype default
delete do double else enum explicit extern false float for friend if inline int long mutable namespace new noexcept
nullptr operator private protected public return short signed sizeof static static_cast struct switch template this
throw true try typedef typename union unsigned using virtual void volatile while size_t""".split())

TOKEN = re.compile(r"""
 (?P<com>//[^\n]*)
|(?P<raw>R"\((?:.|\n)*?\)")
|(?P<str>"(?:\\.|[^"\\\n])*")
|(?P<chr>'(?:\\.|[^'\\\n])')
|(?P<pp>^[ \t]*\#[ \t]*[a-z]+)
|(?P<num>\b\d[\d.']*(?:[eE][+-]?\d+)?[fFuUlL]*\b)
|(?P<id>[A-Za-z_][A-Za-z0-9_]*)
""", re.X | re.M)


def hl_cpp(src):
    out, pos = [], 0
    for m in TOKEN.finditer(src):
        out.append(html.escape(src[pos:m.start()]))
        kind, text = m.lastgroup, m.group(0)
        esc = html.escape(text)
        if kind == "com":
            out.append(f'<i class="c">{esc}</i>')
        elif kind in ("str", "raw", "chr"):
            out.append(f'<b class="s">{esc}</b>')
        elif kind == "pp":
            out.append(f'<b class="p">{esc}</b>')
        elif kind == "num":
            out.append(f'<b class="n">{esc}</b>')
        elif text in KEYWORDS:
            out.append(f'<b class="k">{esc}</b>')
        elif text in ("llm", "std", "json"):
            out.append(f'<b class="ns">{esc}</b>')
        elif re.match(r"LLM_[A-Z_]+", text):
            out.append(f'<b class="m">{esc}</b>')
        else:
            out.append(esc)
        pos = m.end()
    out.append(html.escape(src[pos:]))
    return "".join(out)


def numbered(src_html):
    lines = src_html.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    return "".join(f'<span class="ln">{line}\n</span>' for line in lines)


def hdr_file(name):
    return name.replace("-", "_") + ".hpp"


def read(path):
    with open(os.path.join(ROOT, path), encoding="utf-8") as f:
        return f.read()


def term_out(text):
    rows = []
    for line in text.rstrip("\n").split("\n"):
        e = html.escape(line)
        if line.startswith(("blocked:", "error:")) or "(blocked)" in line:
            e = f'<span class="t-warn">{e}</span>'
        elif line.startswith("valid: yes"):
            e = f'<span class="t-ok">{e}</span>'
        rows.append(e)
    return "\n".join(rows)


# ---------------------------------------------------------------- pieces

def hero_file():
    h = DATA["hero"]
    src = "\n".join(h["lines"])
    return numbered(hl_cpp(src))


def bars():
    mx = max(l["lines"] for l in LIBS)
    out = []
    for l in sorted(LIBS, key=lambda x: -x["lines"]):
        pct = l["lines"] / mx * 100
        out.append(
            f'<li class="bar {l["deps"]}" style="--h:{pct:.1f}%" title="{hdr_file(l["name"])}: {l["lines"]} lines">'
            f'<span class="bar-fill"></span><span class="bar-n">{l["lines"]}</span>'
            f'<span class="bar-l">{l["name"][4:]}</span></li>')
    return "".join(out)


def cards():
    mx = max(l["lines"] for l in LIBS)
    demo = {lib for _, lib, _ in EXAMPLES}
    out = []
    for l in LIBS:
        f = hdr_file(l["name"])
        url = RAW.format(name=l["name"], file=f)
        needs = "libcurl" if l["deps"] == "curl" else "none"
        note = ""
        if l["needs"] not in ("none", "libcurl"):
            note = f'<p class="card-note">{html.escape(l["needs"])}</p>'
        run = ""
        if l["name"] in demo:
            short = l["name"][4:]
            run = f'<a class="btn ghost" href="#ex-{short}">run output</a>'
        out.append(f'''<article class="card" data-name="{l["name"]}" data-cat="{l["cat"]}" data-deps="{l["deps"]}" data-text="{html.escape((l["name"] + " " + l["desc"]).lower())}">
  <header class="card-tab"><span class="card-file">{f}</span><span class="card-lines">{l["lines"]} lines</span></header>
  <div class="card-meter"><span style="width:{l["lines"] / mx * 100:.1f}%"></span></div>
  <h3><a href="{REPO.format(name=l["name"])}">{l["name"]}</a></h3>
  <p class="card-desc">{html.escape(l["desc"])}</p>
  {note}
  <dl class="card-meta"><div><dt>needs</dt><dd class="dep-{l["deps"]}">{needs}</dd></div><div><dt>group</dt><dd>{CAT_LABEL[l["cat"]].split()[0].lower()}</dd></div></dl>
  <code class="card-macro">#define {l["macro"]}</code>
  <div class="card-actions">
    <button class="btn pick" type="button" aria-pressed="false" data-pick="{l["name"]}"><span class="pick-box" aria-hidden="true"></span><span class="pick-label">add</span></button>
    <button class="btn ghost" type="button" data-copy="curl -fsSLO {url}">copy curl</button>
    {run}
  </div>
</article>''')
    return "\n".join(out)


def recipes():
    out = []
    for r in DATA["recipes"]:
        out.append(f'<button type="button" class="recipe" data-libs="{" ".join(r["libs"])}"><span class="recipe-ask">{html.escape(r["ask"])}</span><span class="recipe-use">{html.escape(r["text"])}</span></button>')
    return "".join(out)


def examples():
    tabs, panels = [], []
    for i, (short, lib, blurb) in enumerate(EXAMPLES):
        code = read(f"examples/offline/{short}.cpp")
        outp = read(f"examples/offline/output/{short}.txt")
        sel = "true" if i == 0 else "false"
        lines = next(l["lines"] for l in LIBS if l["name"] == lib)
        tabs.append(f'<button role="tab" id="tab-{short}" aria-controls="ex-{short}" aria-selected="{sel}" tabindex="{0 if i == 0 else -1}"><span>{hdr_file(lib)}</span></button>')
        panels.append(f'''<section class="ex" id="ex-{short}" role="tabpanel" aria-labelledby="tab-{short}" {"" if i == 0 else "hidden"}>
  <p class="ex-blurb"><a href="{REPO.format(name=lib)}">{lib}</a> <span class="dim">({lines} lines, no dependencies)</span>. {html.escape(blurb)}</p>
  <div class="ex-grid">
    <figure class="editor">
      <figcaption><span>{short}.cpp</span><button class="btn tiny" type="button" data-copy-from="src-{short}">copy</button></figcaption>
      <pre class="code" id="src-{short}"><code>{numbered(hl_cpp(code))}</code></pre>
    </figure>
    <figure class="term">
      <figcaption><span class="dots" aria-hidden="true"><i></i><i></i><i></i></span><span>x64 Native Tools</span></figcaption>
      <pre><span class="t-prompt">C:\\demo&gt;</span> cl /nologo /std:c++17 /EHsc /O2 {short}.cpp
{short}.cpp
<span class="t-prompt">C:\\demo&gt;</span> {short}.exe
{term_out(outp)}
<span class="t-prompt">C:\\demo&gt;</span> <span class="cursor"></span></pre>
    </figure>
  </div>
</section>''')
    return "".join(tabs), "\n".join(panels)


def hero_term():
    outp = read("examples/offline/output/cache.txt")
    url = RAW.format(name="llm-cache", file="llm_cache.hpp")
    return (f'<span class="t-prompt">C:\\demo&gt;</span> curl -fsSLO {html.escape(url)}\n'
            f'<span class="t-prompt">C:\\demo&gt;</span> cl /nologo /std:c++17 /EHsc cache.cpp &amp;&amp; cache.exe\n'
            f'cache.cpp\n{term_out(outp)}')


def main():
    n = len(LIBS)
    total = sum(l["lines"] for l in LIBS)
    offline = sum(1 for l in LIBS if l["deps"] == "none")
    tabs, panels = examples()
    lib_json = json.dumps([{"n": l["name"], "d": l["deps"], "m": l["macro"]} for l in LIBS])
    page = TEMPLATE
    for k, v in {
        "N": str(n), "TOTAL": f"{total:,}", "OFFLINE": str(offline), "CURL": str(n - offline),
        "HERO_FILE": hero_file(), "HERO_TOTAL": str(DATA["hero"]["total"]), "HERO_TERM": hero_term(),
        "BARS": bars(), "CARDS": cards(), "RECIPES": recipes(), "TABS": tabs, "PANELS": panels,
        "LIBJSON": lib_json, "BUILD_DATE": BUILD_DATE, "COMPILER": COMPILER,
        "MAXLINES": str(max(l["lines"] for l in LIBS)), "MINLINES": str(min(l["lines"] for l in LIBS)),
    }.items():
        page = page.replace("{{" + k + "}}", v)
    left = re.findall(r"\{\{[A-Z_]+\}\}", page)
    assert not left, left
    assert "\u2014" not in page, "no em dashes"
    os.makedirs(os.path.join(ROOT, "docs"), exist_ok=True)
    with open(os.path.join(ROOT, "docs", "index.html"), "w", encoding="utf-8", newline="\n") as f:
        f.write(page)
    print(f"docs/index.html: {len(page):,} bytes, {n} libraries, {total:,} header lines")


TEMPLATE = open(os.path.join(ROOT, "tools", "site_template.html"), encoding="utf-8").read()

if __name__ == "__main__":
    main()
