#define LLM_EMBED_IMPLEMENTATION
#include "llm_embed.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::EmbedConfig cfg; cfg.api_key = key;
    llm::VectorStore store("faq.bin");

    // Build a searchable FAQ knowledge base
    std::vector<std::pair<std::string, std::string>> faq = {
        {"refund_policy",   "We offer full refunds within 30 days of purchase. No questions asked."},
        {"shipping_time",   "Standard shipping takes 5–7 business days. Express is 1–2 days."},
        {"payment_methods", "We accept Visa, Mastercard, PayPal, and bank transfers."},
        {"cancel_order",    "Orders can be cancelled within 24 hours. Email support@example.com."},
        {"track_order",     "Use the tracking number in your confirmation email on our tracking page."},
        {"contact_support", "Support is available Mon–Fri 9am–5pm EST at support@example.com or +1-800-000."},
        {"warranty",        "All products carry a 12-month manufacturer warranty against defects."},
        {"return_process",  "To return an item: repack it, print the return label from your account, drop it off."},
    };

    // Batch embed all FAQ entries
    std::vector<std::string> texts;
    for (const auto& [id, text] : faq) texts.push_back(text);
    std::cout << "Batch embedding " << texts.size() << " FAQ entries...\n";
    auto embeddings = llm::embed_batch(texts, cfg);

    for (size_t i = 0; i < faq.size(); ++i)
        store.add(faq[i].first, faq[i].second, embeddings[i]);

    // Interactive-style queries
    std::vector<std::string> queries = {
        "Can I get my money back?",
        "How long does delivery take?",
        "My item arrived broken, what do I do?",
    };

    std::cout << std::fixed << std::setprecision(3) << "\n";
    for (const auto& q : queries) {
        std::cout << "Q: " << q << "\n";
        auto qe = llm::embed(q, cfg);
        auto res = store.search(qe, 1);
        if (!res.empty())
            std::cout << "A: " << res[0].entry.text
                      << "  (sim=" << res[0].similarity << ")\n\n";
    }
    return 0;
}
