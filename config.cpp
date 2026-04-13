#include "config.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

// ── tiny hand-rolled JSON helpers ────────────────────────────────────────────
static std::string jsonStr(const std::string& s) {
    return "\"" + s + "\"";
}
static std::string jsonKV(const std::string& k, const std::string& v) {
    return jsonStr(k) + ": " + jsonStr(v);
}
static std::string jsonKVi(const std::string& k, int v) {
    return jsonStr(k) + ": " + std::to_string(v);
}
static std::string jsonKVf(const std::string& k, float v) {
    char buf[32]; snprintf(buf, sizeof(buf), "%.3f", v);
    return jsonStr(k) + ": " + buf;
}

// ── simple reader ─────────────────────────────────────────────────────────────
static std::string extractStr(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\": \"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    auto end = json.find('"', pos);
    return json.substr(pos, end - pos);
}
static int extractInt(const std::string& json, const std::string& key, int def) {
    std::string needle = "\"" + key + "\": ";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return def;
    pos += needle.size();
    return std::stoi(json.substr(pos));
}
static float extractFloat(const std::string& json, const std::string& key, float def) {
    std::string needle = "\"" + key + "\": ";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return def;
    pos += needle.size();
    return std::stof(json.substr(pos));
}

// ─────────────────────────────────────────────────────────────────────────────
std::string Config::configPath() {
    const char* home = std::getenv("HOME");
    std::string dir = home ? std::string(home) + "/.nespc" : ".";
    fs::create_directories(dir);
    return dir + "/config.json";
}

Config Config::load() {
    Config c;
    const char* home = std::getenv("HOME");
    c.romDirectory = home ? std::string(home) : ".";

    // Player 2 defaults
    c.players[1].keys = {
        SDLK_k,       // A
        SDLK_l,       // B
        SDLK_RSHIFT,  // Select
        SDLK_RETURN,  // Start  (shared is fine for P2 default)
        SDLK_w,       // Up
        SDLK_s,       // Down
        SDLK_a,       // Left
        SDLK_d        // Right
    };

    std::ifstream f(configPath());
    if (!f.is_open()) return c;

    std::ostringstream ss; ss << f.rdbuf();
    std::string json = ss.str();

    std::string dir = extractStr(json, "romDirectory");
    if (!dir.empty()) c.romDirectory = dir;
    c.windowScale  = extractInt  (json, "windowScale",  2);
    c.audioVolume  = extractFloat(json, "audioVolume",  0.8f);
    c.fullscreen   = extractInt  (json, "fullscreen",   0) != 0;

    // Key bindings – stored as SDL keycode integers
    for (int p = 0; p < 2; p++) {
        for (int b = 0; b < BTN_COUNT; b++) {
            std::string key = "p" + std::to_string(p) + "_btn" + std::to_string(b);
            int code = extractInt(json, key, (int)c.players[p].keys[b]);
            c.players[p].keys[b] = (SDL_Keycode)code;
        }
    }

    // Recent ROMs
    std::string needle = "\"recentRoms\": [";
    auto pos = json.find(needle);
    if (pos != std::string::npos) {
        pos += needle.size();
        auto end = json.find(']', pos);
        std::string arr = json.substr(pos, end - pos);
        size_t i = 0;
        while (i < arr.size()) {
            auto q1 = arr.find('"', i);
            if (q1 == std::string::npos) break;
            auto q2 = arr.find('"', q1 + 1);
            c.recentRoms.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
            i = q2 + 1;
        }
    }

    return c;
}

void Config::save() const {
    std::ofstream f(configPath());
    f << "{\n";
    f << "  " << jsonKV ("romDirectory", romDirectory) << ",\n";
    f << "  " << jsonKVi("windowScale",  windowScale)  << ",\n";
    f << "  " << jsonKVf("audioVolume",  audioVolume)  << ",\n";
    f << "  " << jsonKVi("fullscreen",   fullscreen ? 1 : 0) << ",\n";

    for (int p = 0; p < 2; p++) {
        for (int b = 0; b < BTN_COUNT; b++) {
            std::string key = "p" + std::to_string(p) + "_btn" + std::to_string(b);
            f << "  " << jsonKVi(key, (int)players[p].keys[b]) << ",\n";
        }
    }

    f << "  \"recentRoms\": [";
    for (size_t i = 0; i < recentRoms.size(); i++) {
        f << jsonStr(recentRoms[i]);
        if (i + 1 < recentRoms.size()) f << ", ";
    }
    f << "]\n}\n";
}
