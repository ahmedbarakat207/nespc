#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "config.h"

enum class GuiScreen {
    Launcher,
    Settings,
    None      // user launched a game – exit GUI loop
};

enum class SettingsTab {
    Controls,
    Emulator
};

struct GUI {
    // Returns path to ROM to load, or "" if user quit
    static std::string run(SDL_Window* win, SDL_Renderer* ren, Config& cfg);

private:
    // ── rendering ─────────────────────────────────────────────────────────────
    static void drawRect     (SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c);
    static void drawRectLine (SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c);
    static void drawText     (SDL_Renderer* r, TTF_Font* f, const std::string& txt,
                              int x, int y, SDL_Color c);
    static int  textWidth    (TTF_Font* f, const std::string& txt);

    // ── screens ───────────────────────────────────────────────────────────────
    static GuiScreen runLauncher (SDL_Window* win, SDL_Renderer* r, TTF_Font* fl, TTF_Font* fs,
                                  Config& cfg, std::string& outRom);
    static GuiScreen runSettings (SDL_Window* win, SDL_Renderer* r, TTF_Font* fl, TTF_Font* fs,
                                  Config& cfg);

    // ── helpers ───────────────────────────────────────────────────────────────
    static std::vector<std::string> scanRoms     (const std::string& dir);
    static std::string              openDirDialog();  // macOS native picker
    static void                     applyFullscreen(SDL_Window* win, bool fs);
};
