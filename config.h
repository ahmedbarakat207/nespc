#pragma once
#include <string>
#include <array>
#include <vector>
#include <SDL2/SDL.h>

// NES controller bit positions
static constexpr int BTN_A      = 7;
static constexpr int BTN_B      = 6;
static constexpr int BTN_SELECT = 5;
static constexpr int BTN_START  = 4;
static constexpr int BTN_UP     = 3;
static constexpr int BTN_DOWN   = 2;
static constexpr int BTN_LEFT   = 1;
static constexpr int BTN_RIGHT  = 0;
static constexpr int BTN_COUNT  = 8;

struct PlayerBindings {
    std::array<SDL_Keycode, BTN_COUNT> keys{
        SDLK_z,       // A
        SDLK_x,       // B
        SDLK_RSHIFT,  // Select
        SDLK_RETURN,  // Start
        SDLK_UP,      // Up
        SDLK_DOWN,    // Down
        SDLK_LEFT,    // Left
        SDLK_RIGHT    // Right
    };
};

struct Config {
    std::string            romDirectory;
    int                    windowScale  = 2;   // 1, 2, or 3
    float                  audioVolume  = 0.8f; // 0.0 – 1.0
    bool                   fullscreen   = false; // windowed by default
    std::array<PlayerBindings, 2> players{};
    std::vector<std::string>      recentRoms;

    static Config load();
    void save() const;

private:
    static std::string configPath();
};
