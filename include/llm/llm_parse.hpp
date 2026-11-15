#pragma once

// llm_parse.hpp -- Offline document parser for LLM pipelines.
// Strip HTML, clean markdown, normalize whitespace, chunk text.
// Zero dependencies. Fully offline.
//
// USAGE: #define LLM_PARSE_IMPLEMENTATION in ONE .cpp file.

#include <string>
#include <vector>
#include <cstddef>

namespace llm {

struct ParseConfig {
    bool   normalize_whitespace = true;
    bool   remove_urls          = false;
    bool   remove_emails        = false;
    size_t max_length           = 0;
    std::string url_replacement   = "[URL]";
    std::string email_replacement = "[EMAIL]";
};

// ---------- HTML ----------
struct HTMLParseResult {
    std::string              text;
    std::string              title;
    std::vector<std::string> links;
    std::vector<std::string> headings;
};
HTMLParseResult parse_html(const std::string& html, const ParseConfig& cfg = {});
std::string     strip_html(const std::string& html);

// ---------- Markdown ----------
struct MarkdownParseResult {
    std::string              plain_text;
    std::vector<std::string> code_blocks;
    std::vector<std::string> headings;
    std::vector<std::string> links;
};
MarkdownParseResult parse_markdown(const std::string& md, const ParseConfig& cfg = {});
std::string         strip_markdown(const std::string& md);

// ---------- Plain text ----------
std::string clean_text(const std::string& text, const ParseConfig& cfg = {});

// ---------- Chunking ----------
struct ChunkConfig {
    size_t chunk_size          = 500;
    size_t overlap             = 50;
    bool   split_on_sentences  = true;
    bool   split_on_paragraphs = true;
};
std::vector<std::string> chunk(const std::string& text, const ChunkConfig& cfg = {});

// ---------- Analysis ----------
struct TextStats {
    size_t char_count;
    size_t word_count;
    size_t sentence_count;
    size_t paragraph_count;
    double avg_word_length;
    bool   likely_code;
    bool   likely_html;
    bool   likely_markdown;
};
TextStats analyze(const std::string& text);

} // namespace llm

// ---------------------------------------------------------------------------
#ifdef LLM_PARSE_IMPLEMENTATION

#include <algorithm>
#include <cctype>
#include <sstream>

namespace llm {
namespace detail_parse {

static bool iws(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

static std::string decode_entities(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        if (s[i] == '&') {
            if (s.compare(i, 5, "&amp;") == 0) { out += '&'; i += 5; }
            else if (s.compare(i, 4, "&lt;") == 0) { out += '<'; i += 4; }
            else if (s.compare(i, 4, "&gt;") == 0) { out += '>'; i += 4; }
            else if (s.compare(i, 6, "&quot;") == 0) { out += '"'; i += 6; }
            else if (s.compare(i, 6, "&nbsp;") == 0) { out += ' '; i += 6; }
            else if (s.compare(i, 6, "&#160;") == 0) { out += ' '; i += 6; }
            else { out += s[i++]; }
        } else { out += s[i++]; }
    }
    return out;
}

static std::string normalize_ws(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool last_space = false;
    int  newlines   = 0;
    for (char c : s) {
        if (c == '\n') {
            ++newlines;
            if (newlines <= 2) out += '\n';
            last_space = false;
        } else if (c == '\r') {
            // skip
        } else if (c == ' ' || c == '\t') {
            newlines = 0;
            if (!last_space && !out.empty() && out.back() != '\n') {
                out += ' '; last_space = true;
            }
        } else {
            newlines = 0; last_space = false;
            out += c;
        }
    }
    // trim
    size_t s0 = out.find_first_not_of(" \n");
    size_t s1 = out.find_last_not_of(" \n");
    return (s0 == std::string::npos) ? "" : out.substr(s0, s1 - s0 + 1);
}

static std::string apply_replacements(std::string s, const ParseConfig& cfg) {
    if (cfg.remove_urls) {
        // simple URL replacement
        size_t p = 0;
        while ((p = s.find("http", p)) != std::string::npos) {
            size_t e = p;
            while (e < s.size() && !iws(s[e]) && s[e] != '"' && s[e] != '\'') ++e;
            s.replace(p, e - p, cfg.url_replacement);
            p += cfg.url_replacement.size();
        }
    }
    if (cfg.remove_emails) {
        // look for word@word.word
        std::string out;
        out.reserve(s.size());
        size_t i = 0;
        while (i < s.size()) {
            size_t at = s.find('@', i);
            if (at == std::string::npos) { out += s.substr(i); break; }
            // scan back for start of local part
            size_t lo = at;
            while (lo > i && (std::isalnum((unsigned char)s[lo-1]) || s[lo-1]=='.' || s[lo-1]=='+' || s[lo-1]=='-' || s[lo-1]=='_')) --lo;
            // scan forward for domain
            size_t hi = at + 1;
            while (hi < s.size() && (std::isalnum((unsigned char)s[hi]) || s[hi]=='.' || s[hi]=='-')) ++hi;
            if (hi > at + 2 && lo < at) {
                out += s.substr(i, lo - i);
                out += cfg.email_replacement;
                i = hi;
            } else {
                out += s.substr(i, at - i + 1);
                i = at + 1;
            }
        }
        s = out;
    }
    if (cfg.max_length > 0 && s.size() > cfg.max_length)
        s = s.substr(0, cfg.max_length);
    return s;
}

// Extract attribute value from tag string, e.g. href="..."
static std::string attr_val(const std::string& tag, const std::string& attr) {
    std::string pat = attr + "=";
    auto p = tag.find(pat);
    if (p == std::string::npos) return {};
    p += pat.size();
    char q = (p < tag.size() && (tag[p] == '"' || tag[p] == '\'')) ? tag[p++] : 0;
    size_t e = p;
    if (q) {
        while (e < tag.size() && tag[e] != q) ++e;
    } else {
        while (e < tag.size() && !iws(tag[e]) && tag[e] != '>') ++e;
    }
    return tag.substr(p, e - p);
}

} // namespace detail_parse

// ---------------------------------------------------------------------------
// HTML
// ---------------------------------------------------------------------------

std::string strip_html(const std::string& html) {
    std::string out;
    out.reserve(html.size());
    bool in_tag    = false;
    bool in_script = false;
    bool in_style  = false;
    std::string tag_buf;

    for (size_t i = 0; i < html.size(); ++i) {
        if (html[i] == '<') {
            in_tag = true;
            tag_buf.clear();
        } else if (html[i] == '>' && in_tag) {
            in_tag = false;
            std::string tl = tag_buf;
            std::transform(tl.begin(), tl.end(), tl.begin(),
                           [](unsigned char c){ return (char)std::tolower(c); });
            if (tl.rfind("script", 0) == 0) in_script = true;
            else if (tl.rfind("/script", 0) == 0) in_script = false;
            else if (tl.rfind("style", 0) == 0) in_style = true;
            else if (tl.rfind("/style", 0) == 0) in_style = false;
            else if (!in_script && !in_style) {
                // Block elements → newline
                if (tl == "p" || tl == "br" || tl == "div" || tl == "li" ||
                    tl.rfind("h", 0) == 0) out += '\n';
            }
        } else if (in_tag) {
            tag_buf += html[i];
        } else if (!in_script && !in_style) {
            out += html[i];
        }
    }
    return detail_parse::decode_entities(out);
}

HTMLParseResult parse_html(const std::string& html, const ParseConfig& cfg) {
    HTMLParseResult result;

    bool in_tag     = false;
    bool in_script  = false;
    bool in_style   = false;
    bool in_title   = false;
    bool in_heading = false;
    std::string tag_buf;
    std::string text_buf;

    auto flush_text = [&]() {
        auto decoded = detail_parse::decode_entities(text_buf);
        if (in_title) result.title += decoded;
        else result.text += decoded;
        text_buf.clear();
    };

    for (size_t i = 0; i < html.size(); ++i) {
        if (html[i] == '<') {
            flush_text();
            in_tag = true;
            tag_buf.clear();
        } else if (html[i] == '>' && in_tag) {
            in_tag = false;
            std::string tl = tag_buf;
            std::transform(tl.begin(), tl.end(), tl.begin(),
                           [](unsigned char c){ return (char)std::tolower(c); });
            // script/style
            if (tl.rfind("script", 0) == 0) { in_script = true; }
            else if (tl.rfind("/script", 0) == 0) { in_script = false; }
            else if (tl.rfind("style", 0) == 0) { in_style = true; }
            else if (tl.rfind("/style", 0) == 0) { in_style = false; }
            else if (!in_script && !in_style) {
                // title
                if (tl == "title") in_title = true;
                else if (tl == "/title") in_title = false;
                // headings
                else if (tl.size() == 2 && tl[0] == 'h' && tl[1] >= '1' && tl[1] <= '6') {
                    flush_text();
                    in_heading = true;
                    result.text += '\n';
                }
                else if (tl.size() == 3 && tl[0] == '/' && tl[1] == 'h' && tl[2] >= '1' && tl[2] <= '6') {
                    flush_text();
                    if (!result.text.empty()) {
                        // grab heading text: walk back to last '\n' to find heading content
                        auto last_nl = result.text.rfind('\n', result.text.size() - 1);
                        if (last_nl != std::string::npos) {
                            std::string heading = result.text.substr(last_nl + 1);
                            if (!heading.empty()) result.headings.push_back(heading);
                        }
                    }
                    in_heading = false;
                    result.text += '\n';
                }
                // links
                else if (tl.rfind("a ", 0) == 0 || tl == "a") {
                    auto href = detail_parse::attr_val(tag_buf, "href");
                    if (!href.empty()) result.links.push_back(href);
                }
                // block elements
                else if (tl == "p" || tl == "br" || tl == "/p" ||
                         tl == "div" || tl == "/div" || tl == "li") {
                    result.text += '\n';
                }
            }
        } else if (in_tag) {
            tag_buf += html[i];
        } else if (!in_script && !in_style) {
            text_buf += html[i];
        }
    }
    flush_text();

    // Extract headings from text (crude but avoids second pass)
    // Already handled inline above — just normalize
    if (cfg.normalize_whitespace) result.text = detail_parse::normalize_ws(result.text);
    result.title = detail_parse::normalize_ws(result.title);
    result.text  = detail_parse::apply_replacements(result.text, cfg);
    return result;
}

// ---------------------------------------------------------------------------
// Markdown
// ---------------------------------------------------------------------------

std::string strip_markdown(const std::string& md) {
    std::string out;
    std::istringstream ss(md);
    std::string line;
    bool in_fence = false;

    while (std::getline(ss, line)) {
        // code fences
        if (line.size() >= 3 && line.substr(0, 3) == "```") {
            in_fence = !in_fence;
            continue;
        }
        if (in_fence) { out += line + '\n'; continue; }

        // headings
        size_t hc = 0;
        while (hc < line.size() && line[hc] == '#') ++hc;
        if (hc > 0 && hc < line.size() && line[hc] == ' ') line = line.substr(hc + 1);

        // bold/italic
        for (auto& pat : std::vector<std::string>{"**", "__", "*", "_"}) {
            std::string r;
            size_t p = 0, pl = pat.size();
            while (p < line.size()) {
                auto f = line.find(pat, p);
                if (f == std::string::npos) { r += line.substr(p); break; }
                r += line.substr(p, f - p);
                p = f + pl;
            }
            line = r;
        }

        // inline code `...`
        {
            std::string r;
            size_t p = 0;
            while (p < line.size()) {
                auto f = line.find('`', p);
                if (f == std::string::npos) { r += line.substr(p); break; }
                r += line.substr(p, f - p);
                auto e = line.find('`', f + 1);
                if (e == std::string::npos) { p = f + 1; }
                else { r += line.substr(f + 1, e - f - 1); p = e + 1; }
            }
            line = r;
        }

        // links [text](url) → text
        {
            std::string r;
            size_t p = 0;
            while (p < line.size()) {
                auto f = line.find('[', p);
                if (f == std::string::npos) { r += line.substr(p); break; }
                auto cb = line.find(']', f);
                if (cb == std::string::npos) { r += line.substr(p); break; }
                if (cb + 1 < line.size() && line[cb + 1] == '(') {
                    auto ep = line.find(')', cb + 2);
                    r += line.substr(p, f - p);
                    r += line.substr(f + 1, cb - f - 1); // text only
                    p = (ep == std::string::npos) ? cb + 2 : ep + 1;
                } else {
                    r += line.substr(p, cb + 1 - p); p = cb + 1;
                }
            }
            line = r;
        }

        // list markers
        if (line.size() >= 2 && (line[0] == '-' || line[0] == '*' || line[0] == '+') && line[1] == ' ')
            line = line.substr(2);

        // blockquote
        if (!line.empty() && line[0] == '>') line = line.substr(line.find_first_not_of("> "));

        out += line + '\n';
    }
    return out;
}

MarkdownParseResult parse_markdown(const std::string& md, const ParseConfig& cfg) {
    MarkdownParseResult result;
    std::istringstream ss(md);
    std::string line;
    bool in_fence = false;
    std::string fence_buf;

    while (std::getline(ss, line)) {
        if (line.size() >= 3 && line.substr(0, 3) == "```") {
            if (in_fence) {
                result.code_blocks.push_back(fence_buf);
                fence_buf.clear();
                in_fence = false;
            } else {
                in_fence = true;
            }
            continue;
        }
        if (in_fence) { fence_buf += line + '\n'; continue; }

        // headings
        size_t hc = 0;
        while (hc < line.size() && line[hc] == '#') ++hc;
        if (hc > 0 && hc < line.size() && line[hc] == ' ')
            result.headings.push_back(line.substr(hc + 1));

        // links
        size_t p = 0;
        while (p < line.size()) {
            auto f = line.find('[', p);
            if (f == std::string::npos) break;
            auto cb = line.find(']', f);
            if (cb != std::string::npos && cb + 1 < line.size() && line[cb+1] == '(') {
                auto ep = line.find(')', cb + 2);
                if (ep != std::string::npos)
                    result.links.push_back(line.substr(cb + 2, ep - cb - 2));
                p = (ep == std::string::npos) ? cb + 2 : ep + 1;
            } else { p = f + 1; }
        }
    }
    result.plain_text = strip_markdown(md);
    if (cfg.normalize_whitespace) result.plain_text = detail_parse::normalize_ws(result.plain_text);
    result.plain_text = detail_parse::apply_replacements(result.plain_text, cfg);
    return result;
}

// ---------------------------------------------------------------------------
// Plain text
// ---------------------------------------------------------------------------

std::string clean_text(const std::string& text, const ParseConfig& cfg) {
    std::string s = text;
    if (cfg.normalize_whitespace) s = detail_parse::normalize_ws(s);
    return detail_parse::apply_replacements(s, cfg);
}

// ---------------------------------------------------------------------------
// Chunking
// ---------------------------------------------------------------------------

std::vector<std::string> chunk(const std::string& text, const ChunkConfig& cfg) {
    std::vector<std::string> chunks;
    if (text.empty()) return chunks;

    size_t sz  = cfg.chunk_size;
    size_t ov  = std::min(cfg.overlap, sz / 2);
    size_t pos = 0;
    size_t len = text.size();

    while (pos < len) {
        size_t end = std::min(pos + sz, len);

        if (end < len) {
            // Try to find a paragraph boundary
            if (cfg.split_on_paragraphs) {
                auto pb = text.rfind("\n\n", end);
                if (pb != std::string::npos && pb > pos + sz / 4)
                    end = pb + 2;
            }
            // Otherwise try sentence boundary
            if (end == std::min(pos + sz, len) && cfg.split_on_sentences) {
                size_t sb = text.rfind('.', end);
                if (sb == std::string::npos) sb = text.rfind('!', end);
                if (sb == std::string::npos) sb = text.rfind('?', end);
                if (sb != std::string::npos && sb > pos + sz / 4)
                    end = sb + 1;
            }
        }

        auto c = text.substr(pos, end - pos);
        // trim
        size_t cs = c.find_first_not_of(" \n");
        size_t ce = c.find_last_not_of(" \n");
        if (cs != std::string::npos) chunks.push_back(c.substr(cs, ce - cs + 1));

        if (end >= len) break;
        pos = (end > ov) ? end - ov : 0;
    }
    return chunks;
}

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

TextStats analyze(const std::string& text) {
    TextStats s{};
    s.char_count = text.size();

    // words
    bool in_word = false;
    size_t word_chars = 0;
    for (char c : text) {
        if (std::isspace((unsigned char)c)) {
            if (in_word) { ++s.word_count; in_word = false; }
        } else { in_word = true; ++word_chars; }
    }
    if (in_word) ++s.word_count;
    s.avg_word_length = s.word_count > 0 ? (double)word_chars / s.word_count : 0;

    // sentences
    for (char c : text) if (c == '.' || c == '!' || c == '?') ++s.sentence_count;

    // paragraphs
    size_t dbl = 0;
    for (size_t i = 0; i + 1 < text.size(); ++i)
        if (text[i] == '\n' && text[i+1] == '\n') { ++dbl; ++i; }
    s.paragraph_count = dbl + 1;

    // heuristics
    size_t code_chars = 0, html_chars = 0, md_chars = 0;
    for (char c : text) {
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == ';' || c == '=') ++code_chars;
        if (c == '<' || c == '>') ++html_chars;
        if (c == '#' || c == '*' || c == '`') ++md_chars;
    }
    double density = s.char_count > 0 ? 1.0 / s.char_count : 0;
    s.likely_code     = code_chars * density > 0.02;
    s.likely_html     = html_chars * density > 0.01;
    s.likely_markdown = md_chars  * density > 0.01;
    return s;
}

} // namespace llm
#endif // LLM_PARSE_IMPLEMENTATION
