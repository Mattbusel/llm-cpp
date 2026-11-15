#pragma once
// llm-template: single-header C++ prompt templating
// Syntax: {{var}}, {{#list}}...{{/list}}, {{#flag}}...{{/flag}}, {{!comment}}
// #define LLM_TEMPLATE_IMPLEMENTATION in ONE .cpp before including.

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace llm {

using TemplateVars = std::map<std::string, std::string>;

struct TemplateContext {
    std::map<std::string, std::string>              vars;
    std::map<std::string, std::vector<TemplateVars>> lists;
    std::map<std::string, bool>                      flags;
};

class Template {
public:
    explicit Template(const std::string& tmpl);

    std::string render(const TemplateVars& vars) const;
    std::string render(const TemplateContext& ctx) const;

    // Render and truncate: estimates tokens as chars/4, trims longest var value first
    std::string render_truncated(const TemplateContext& ctx, size_t max_tokens) const;

    std::vector<std::string> variables() const;
    std::vector<std::string> missing_vars(const TemplateContext& ctx) const;

private:
    std::string tmpl_;
};

class TemplateRegistry {
public:
    void     add(const std::string& name, const std::string& tmpl);
    void     load_file(const std::string& name, const std::string& filepath);
    Template& get(const std::string& name);
    bool     has(const std::string& name) const;
private:
    std::map<std::string, Template> registry_;
};

} // namespace llm

#ifdef LLM_TEMPLATE_IMPLEMENTATION

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace llm {
namespace detail {

// Render one pass: handle {{!comment}}, {{var}}, {{#flag}}...{{/flag}}, {{#list}}...{{/list}}
static std::string render_impl(const std::string& tmpl, const TemplateContext& ctx) {
    std::string out;
    size_t i = 0;
    while (i < tmpl.size()) {
        if (tmpl[i] == '{' && i+1 < tmpl.size() && tmpl[i+1] == '{') {
            size_t end = tmpl.find("}}", i+2);
            if (end == std::string::npos) { out += tmpl[i++]; continue; }
            std::string tag = tmpl.substr(i+2, end-i-2);
            i = end + 2;

            if (!tag.empty() && tag[0] == '!') {
                // comment — skip
            } else if (!tag.empty() && tag[0] == '#') {
                std::string name = tag.substr(1);
                // find closing {{/name}}
                std::string close_tag = "{{/" + name + "}}";
                size_t close_pos = tmpl.find(close_tag, i);
                if (close_pos == std::string::npos) continue;
                std::string inner = tmpl.substr(i, close_pos - i);
                i = close_pos + close_tag.size();

                // Check if it's a list
                auto lit = ctx.lists.find(name);
                if (lit != ctx.lists.end()) {
                    for (const auto& row : lit->second) {
                        // Replace {{item.key}} inside inner
                        std::string row_out = inner;
                        for (const auto& kv : row) {
                            std::string placeholder = "{{" + kv.first + "}}";
                            size_t p = 0;
                            while ((p = row_out.find(placeholder, p)) != std::string::npos) {
                                row_out.replace(p, placeholder.size(), kv.second);
                                p += kv.second.size();
                            }
                        }
                        // also render global vars
                        TemplateContext sub_ctx{row, {}, {}};
                        out += render_impl(row_out, sub_ctx);
                    }
                } else {
                    // conditional flag
                    auto fit = ctx.flags.find(name);
                    if (fit != ctx.flags.end() && fit->second)
                        out += render_impl(inner, ctx);
                }
            } else if (!tag.empty() && tag[0] == '>') {
                // partial — skip (no registry available here)
                out += "[partial:" + tag.substr(1) + "]";
            } else {
                // variable
                auto it = ctx.vars.find(tag);
                if (it != ctx.vars.end()) out += it->second;
                // else leave empty
            }
        } else {
            out += tmpl[i++];
        }
    }
    return out;
}

static size_t estimate_tokens(const std::string& s) { return (s.size() + 3) / 4; }

static std::vector<std::string> find_vars(const std::string& tmpl) {
    std::vector<std::string> vars;
    size_t i = 0;
    while (i < tmpl.size()) {
        if (tmpl[i] == '{' && i+1 < tmpl.size() && tmpl[i+1] == '{') {
            size_t end = tmpl.find("}}", i+2);
            if (end == std::string::npos) { ++i; continue; }
            std::string tag = tmpl.substr(i+2, end-i-2);
            i = end + 2;
            if (!tag.empty() && tag[0] != '!' && tag[0] != '#' && tag[0] != '/' && tag[0] != '>')
                if (std::find(vars.begin(), vars.end(), tag) == vars.end())
                    vars.push_back(tag);
        } else { ++i; }
    }
    return vars;
}

} // namespace detail

Template::Template(const std::string& tmpl) : tmpl_(tmpl) {}

std::string Template::render(const TemplateVars& vars) const {
    TemplateContext ctx; ctx.vars = vars;
    return detail::render_impl(tmpl_, ctx);
}

std::string Template::render(const TemplateContext& ctx) const {
    return detail::render_impl(tmpl_, ctx);
}

std::string Template::render_truncated(const TemplateContext& ctx, size_t max_tokens) const {
    // Try rendering as-is first
    std::string result = detail::render_impl(tmpl_, ctx);
    if (detail::estimate_tokens(result) <= max_tokens) return result;

    // Find the longest var and trim it iteratively
    TemplateContext working = ctx;
    while (detail::estimate_tokens(result) > max_tokens) {
        // find longest value
        std::string longest_key;
        size_t longest_len = 0;
        for (auto& kv : working.vars) {
            if (kv.second.size() > longest_len) { longest_len = kv.second.size(); longest_key = kv.first; }
        }
        if (longest_key.empty() || longest_len == 0) break;
        // trim by 10%
        size_t new_len = longest_len * 9 / 10;
        if (new_len == longest_len) new_len = longest_len > 4 ? longest_len - 4 : 0;
        working.vars[longest_key] = working.vars[longest_key].substr(0, new_len) + "...";
        result = detail::render_impl(tmpl_, working);
    }
    return result;
}

std::vector<std::string> Template::variables() const {
    return detail::find_vars(tmpl_);
}

std::vector<std::string> Template::missing_vars(const TemplateContext& ctx) const {
    std::vector<std::string> missing;
    for (const auto& v : detail::find_vars(tmpl_))
        if (ctx.vars.find(v) == ctx.vars.end()) missing.push_back(v);
    return missing;
}

void TemplateRegistry::add(const std::string& name, const std::string& tmpl) {
    registry_.emplace(name, Template(tmpl));
}

void TemplateRegistry::load_file(const std::string& name, const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f) throw std::runtime_error("Cannot open template file: " + filepath);
    std::ostringstream ss; ss << f.rdbuf();
    add(name, ss.str());
}

Template& TemplateRegistry::get(const std::string& name) {
    auto it = registry_.find(name);
    if (it == registry_.end()) throw std::runtime_error("Template not found: " + name);
    return it->second;
}

bool TemplateRegistry::has(const std::string& name) const {
    return registry_.find(name) != registry_.end();
}

} // namespace llm
#endif // LLM_TEMPLATE_IMPLEMENTATION
