#pragma once
#define NOMINMAX
// llm_audio.hpp — Single-header C++ audio transcription, translation, and TTS via OpenAI
// MIT License — Mattbusel, 2026
// Usage: #define LLM_AUDIO_IMPLEMENTATION in ONE .cpp before including

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace llm {

// ── Speech to Text ──────────────────────────────────────────────────────────

struct TranscribeConfig {
    std::string api_key;
    std::string model            = "whisper-1";
    std::string language         = "";       // "" = auto-detect
    std::string prompt           = "";       // optional context hint
    std::string response_format  = "text";   // "text","json","srt","vtt"
    double      temperature      = 0.0;
    long        timeout_secs     = 120;      // audio uploads can be slow
};

struct TranscribeResult {
    std::string text;
    std::string language;
    double      duration_seconds = 0.0;
    std::string format;
};

// Transcribe an audio file (mp3, mp4, wav, m4a, ogg, webm) to text.
TranscribeResult transcribe(const std::string& filepath,
                             const TranscribeConfig& config);

// Transcribe from raw bytes.
TranscribeResult transcribe_bytes(const std::vector<uint8_t>& audio_bytes,
                                   const std::string& filename,
                                   const TranscribeConfig& config);

// ── Translation ──────────────────────────────────────────────────────────────
// Translates audio in any supported language into English text.

struct TranslateConfig {
    std::string api_key;
    std::string model            = "whisper-1";
    std::string prompt           = "";       // optional English context hint
    std::string response_format  = "text";   // "text","json","srt","vtt"
    double      temperature      = 0.0;
    long        timeout_secs     = 120;
};

TranscribeResult translate(const std::string& filepath,
                            const TranslateConfig& config);

TranscribeResult translate_bytes(const std::vector<uint8_t>& audio_bytes,
                                  const std::string& filename,
                                  const TranslateConfig& config);

// ── Text to Speech ───────────────────────────────────────────────────────────

enum class TTSVoice  { Alloy, Echo, Fable, Onyx, Nova, Shimmer };
enum class TTSFormat { MP3, Opus, AAC, FLAC };

struct TTSConfig {
    std::string api_key;
    std::string model        = "tts-1";
    TTSVoice    voice        = TTSVoice::Alloy;
    TTSFormat   format       = TTSFormat::MP3;
    double      speed        = 1.0;   // 0.25–4.0
    long        timeout_secs = 60;
};

// Generate speech and save to file.
void text_to_speech(const std::string& text,
                    const std::string& output_filepath,
                    const TTSConfig& config);

// Generate speech and return raw bytes.
std::vector<uint8_t> text_to_speech_bytes(const std::string& text,
                                           const TTSConfig& config);

} // namespace llm

// ─────────────────────────────────────────────────────────────────────────────
#ifdef LLM_AUDIO_IMPLEMENTATION

#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <stdexcept>

namespace llm {
namespace detail_audio {

struct CurlH {
    CURL* h;
    explicit CurlH() : h(curl_easy_init()) {
        if (!h) throw std::runtime_error("curl_easy_init failed");
    }
    ~CurlH() { curl_easy_cleanup(h); }
    CurlH(const CurlH&)            = delete;
    CurlH& operator=(const CurlH&) = delete;
};

struct SlH {
    curl_slist* s = nullptr;
    ~SlH() { if (s) curl_slist_free_all(s); }
    SlH(const SlH&)            = delete;
    SlH& operator=(const SlH&) = delete;
    SlH()                      = default;
};

static size_t write_vec(void* ptr, size_t size, size_t nmemb, void* udata) {
    auto* buf = static_cast<std::vector<uint8_t>*>(udata);
    auto* p   = static_cast<uint8_t*>(ptr);
    buf->insert(buf->end(), p, p + size * nmemb);
    return size * nmemb;
}

static size_t write_str(void* ptr, size_t size, size_t nmemb, void* udata) {
    static_cast<std::string*>(udata)->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static std::string jstr(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos); if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        ++pos;
        std::string out;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) { ++pos; }
            out += json[pos++];
        }
        return out;
    }
    size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') ++end;
    std::string v = json.substr(pos, end - pos);
    auto ts = v.find_first_not_of(" \t\r\n");
    auto te = v.find_last_not_of(" \t\r\n");
    return (ts == std::string::npos) ? "" : v.substr(ts, te - ts + 1);
}

static const char* tts_voice_str(TTSVoice v) {
    switch (v) {
        case TTSVoice::Alloy:   return "alloy";
        case TTSVoice::Echo:    return "echo";
        case TTSVoice::Fable:   return "fable";
        case TTSVoice::Onyx:    return "onyx";
        case TTSVoice::Nova:    return "nova";
        case TTSVoice::Shimmer: return "shimmer";
    }
    return "alloy";
}

static const char* tts_format_str(TTSFormat f) {
    switch (f) {
        case TTSFormat::MP3:  return "mp3";
        case TTSFormat::Opus: return "opus";
        case TTSFormat::AAC:  return "aac";
        case TTSFormat::FLAC: return "flac";
    }
    return "mp3";
}

struct MultipartBuilder {
    std::string boundary = "----LLMAudioBoundary7f3a9b";
    std::vector<uint8_t> body;

    void add_field(const std::string& name, const std::string& value) {
        std::string part = "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n"
            + value + "\r\n";
        body.insert(body.end(), part.begin(), part.end());
    }

    void add_file(const std::string& name, const std::string& filename,
                  const std::vector<uint8_t>& data) {
        std::string header = "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"" + name +
            "\"; filename=\"" + filename + "\"\r\n"
            "Content-Type: application/octet-stream\r\n\r\n";
        body.insert(body.end(), header.begin(), header.end());
        body.insert(body.end(), data.begin(), data.end());
        const char* crlf = "\r\n";
        body.insert(body.end(), crlf, crlf + 2);
    }

    std::string content_type() const { return "multipart/form-data; boundary=" + boundary; }

    void finish() {
        std::string tail = "--" + boundary + "--\r\n";
        body.insert(body.end(), tail.begin(), tail.end());
    }
};

// Shared POST helper for both transcribe and translate endpoints.
static TranscribeResult do_audio_post(const std::vector<uint8_t>& audio,
                                       const std::string& filename,
                                       const std::string& endpoint_url,
                                       const std::string& api_key,
                                       const std::string& model,
                                       const std::string& response_format,
                                       const std::string& language,
                                       const std::string& prompt,
                                       double temperature,
                                       long timeout_secs) {
    MultipartBuilder mp;
    mp.add_file("file", filename, audio);
    mp.add_field("model", model);
    mp.add_field("response_format", response_format);
    if (!language.empty()) mp.add_field("language", language);
    if (!prompt.empty())   mp.add_field("prompt", prompt);
    if (temperature != 0.0) mp.add_field("temperature", std::to_string(temperature));
    mp.finish();

    CurlH curl;
    SlH sl;
    std::string auth = "Authorization: Bearer " + api_key;
    std::string ct   = "Content-Type: " + mp.content_type();
    sl.s = curl_slist_append(sl.s, auth.c_str());
    sl.s = curl_slist_append(sl.s, ct.c_str());

    std::string response;
    curl_easy_setopt(curl.h, CURLOPT_URL, endpoint_url.c_str());
    curl_easy_setopt(curl.h, CURLOPT_HTTPHEADER, sl.s);
    curl_easy_setopt(curl.h, CURLOPT_POST, 1L);
    curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS, mp.body.data());
    curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE, (long)mp.body.size());
    curl_easy_setopt(curl.h, CURLOPT_WRITEFUNCTION, write_str);
    curl_easy_setopt(curl.h, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.h, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl.h, CURLOPT_TIMEOUT, timeout_secs);

    CURLcode rc = curl_easy_perform(curl.h);
    if (rc != CURLE_OK) throw std::runtime_error(curl_easy_strerror(rc));

    if (response.find("\"error\"") != std::string::npos) {
        std::string msg = jstr(response, "message");
        throw std::runtime_error("API error: " + (msg.empty() ? response : msg));
    }

    TranscribeResult r;
    r.format = response_format;
    if (response_format == "json") {
        r.text     = jstr(response, "text");
        r.language = jstr(response, "language");
        std::string dur = jstr(response, "duration");
        if (!dur.empty()) {
            try { r.duration_seconds = std::stod(dur); } catch (...) {}
        }
    } else {
        r.text = response;
        while (!r.text.empty() && (r.text.back() == '\n' || r.text.back() == '\r'))
            r.text.pop_back();
    }
    return r;
}

static std::vector<uint8_t> read_file(const std::string& filepath) {
    std::ifstream f(filepath, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + filepath);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
}

static std::string basename(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

} // namespace detail_audio

TranscribeResult transcribe(const std::string& filepath,
                             const TranscribeConfig& config) {
    auto bytes = detail_audio::read_file(filepath);
    return detail_audio::do_audio_post(
        bytes, detail_audio::basename(filepath),
        "https://api.openai.com/v1/audio/transcriptions",
        config.api_key, config.model, config.response_format,
        config.language, config.prompt, config.temperature, config.timeout_secs);
}

TranscribeResult transcribe_bytes(const std::vector<uint8_t>& audio_bytes,
                                   const std::string& filename,
                                   const TranscribeConfig& config) {
    return detail_audio::do_audio_post(
        audio_bytes, filename,
        "https://api.openai.com/v1/audio/transcriptions",
        config.api_key, config.model, config.response_format,
        config.language, config.prompt, config.temperature, config.timeout_secs);
}

TranscribeResult translate(const std::string& filepath,
                            const TranslateConfig& config) {
    auto bytes = detail_audio::read_file(filepath);
    return detail_audio::do_audio_post(
        bytes, detail_audio::basename(filepath),
        "https://api.openai.com/v1/audio/translations",
        config.api_key, config.model, config.response_format,
        "", config.prompt, config.temperature, config.timeout_secs);
}

TranscribeResult translate_bytes(const std::vector<uint8_t>& audio_bytes,
                                  const std::string& filename,
                                  const TranslateConfig& config) {
    return detail_audio::do_audio_post(
        audio_bytes, filename,
        "https://api.openai.com/v1/audio/translations",
        config.api_key, config.model, config.response_format,
        "", config.prompt, config.temperature, config.timeout_secs);
}

void text_to_speech(const std::string& text,
                    const std::string& output_filepath,
                    const TTSConfig& config) {
    auto bytes = text_to_speech_bytes(text, config);
    std::ofstream f(output_filepath, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot write file: " + output_filepath);
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}

std::vector<uint8_t> text_to_speech_bytes(const std::string& text,
                                           const TTSConfig& config) {
    auto jesc = [](const std::string& s) {
        std::string out; out.reserve(s.size() + 4);
        for (char c : s) {
            if      (c == '"')  out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else                out += c;
        }
        return out;
    };

    std::string body = "{\"model\":\"" + config.model + "\","
        "\"input\":\"" + jesc(text) + "\","
        "\"voice\":\"" + detail_audio::tts_voice_str(config.voice) + "\","
        "\"response_format\":\"" + detail_audio::tts_format_str(config.format) + "\","
        "\"speed\":" + std::to_string(config.speed) + "}";

    detail_audio::CurlH curl;
    detail_audio::SlH sl;
    sl.s = curl_slist_append(sl.s, ("Authorization: Bearer " + config.api_key).c_str());
    sl.s = curl_slist_append(sl.s, "Content-Type: application/json");

    std::vector<uint8_t> result;
    curl_easy_setopt(curl.h, CURLOPT_URL, "https://api.openai.com/v1/audio/speech");
    curl_easy_setopt(curl.h, CURLOPT_HTTPHEADER, sl.s);
    curl_easy_setopt(curl.h, CURLOPT_POST, 1L);
    curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE, (long)body.size());
    curl_easy_setopt(curl.h, CURLOPT_WRITEFUNCTION, detail_audio::write_vec);
    curl_easy_setopt(curl.h, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl.h, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl.h, CURLOPT_TIMEOUT, config.timeout_secs);

    CURLcode rc = curl_easy_perform(curl.h);
    if (rc != CURLE_OK) throw std::runtime_error(curl_easy_strerror(rc));

    // TTS returns binary audio on success; errors come back as JSON
    if (result.size() > 10) {
        std::string prefix(result.begin(), result.begin() + std::min(result.size(), (size_t)20));
        if (prefix.find("{\"error\"") != std::string::npos) {
            std::string resp(result.begin(), result.end());
            std::string msg = detail_audio::jstr(resp, "message");
            throw std::runtime_error("API error: " + (msg.empty() ? resp : msg));
        }
    }
    return result;
}

} // namespace llm

#endif // LLM_AUDIO_IMPLEMENTATION
