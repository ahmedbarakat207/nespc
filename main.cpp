#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>
#include "bus.h"
#include "config.h"
#include "gui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static std::vector<float> audioBuffer;
static std::mutex         audioMutex;
static const int          AUDIO_BUFFER_SIZE = 2048;
static float              gAudioVolume      = 0.8f;

static void audioCallback(void*, Uint8* stream, int len) {
    float* out    = reinterpret_cast<float*>(stream);
    int    frames = len / sizeof(float);
    std::lock_guard<std::mutex> lock(audioMutex);
    int available = (int)audioBuffer.size();
    for (int i = 0; i < frames; i++) {
        float s = (i < available) ? audioBuffer[i] : 0.0f;
        out[i] = s * gAudioVolume;
    }
    if (available >= frames)
        audioBuffer.erase(audioBuffer.begin(), audioBuffer.begin() + frames);
    else
        audioBuffer.clear();
}

// Returns false if user quit without launching anything
static bool runEmulator(SDL_Window* win, SDL_Renderer* ren,
                        SDL_AudioDeviceID audioDevice,
                        const std::string& romPath, Config& cfg) {
    // Apply configured window size (or fullscreen)
    SDL_SetWindowFullscreen(win, 0); // leave GUI fullscreen first
    int scale = std::max(1, std::min(cfg.windowScale, 3));
    if (cfg.fullscreen) {
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowSize(win, 256 * scale, 240 * scale);
        SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    SDL_SetWindowTitle(win, ("NESPC \xe2\x80\x93 " + romPath.substr(romPath.rfind('/') + 1)).c_str());

    // Lock renderer to NES native resolution; SDL scales to fill the window/screen
    SDL_RenderSetLogicalSize(ren, 256, 240);

    SDL_Texture* tex = SDL_CreateTexture(ren,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    if (!tex) return false;

    bus   nes;
    try {
        auto cart = std::make_shared<rom>(romPath);
        nes.insert_cartridge(cart);
        nes.reset();
    } catch (const std::exception& e) {
        fprintf(stderr, "ROM Error: %s\n", e.what());
        SDL_DestroyTexture(tex);
        return false;
    }

    gAudioVolume = cfg.audioVolume;
    SDL_PauseAudioDevice(audioDevice, 0);

    // keys[] is stored as [A, B, Select, Start, Up, Down, Left, Right] at indices 0..7.
    // The bus latches controller[] and reads MSB first: bit7=A, bit6=B, ..., bit0=Right.
    // So key at index b sets bit (7 - b).
    auto applyKey = [&](SDL_Keycode k, bool pressed) {
        for (int p = 0; p < 2; p++) {
            for (int b = 0; b < BTN_COUNT; b++) {
                if (cfg.players[p].keys[b] == k) {
                    uint8_t bit = 1u << (7 - b);
                    if (pressed) nes.controller[p] |= bit;
                    else         nes.controller[p] &= ~bit;
                }
            }
        }
    };

    bool running      = true;
    bool backToMenu   = false;
    SDL_Event ev;

    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = false; backToMenu = false; break; }

            if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                applyKey(k, true);
                if (k == SDLK_r)       nes.reset();
                if (k == SDLK_ESCAPE)  { running = false; backToMenu = true; }
                if (k == SDLK_F11) {
                    cfg.fullscreen = !cfg.fullscreen;
                    cfg.save();
                    if (cfg.fullscreen) {
                        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    } else {
                        SDL_SetWindowFullscreen(win, 0);
                        int sc = std::max(1, std::min(cfg.windowScale, 3));
                        SDL_SetWindowSize(win, 256 * sc, 240 * sc);
                        SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                    }
                }
            }
            if (ev.type == SDL_KEYUP)
                applyKey(ev.key.keysym.sym, false);
        }

        SDL_PumpEvents();

        do {
            nes.clock();
            if (nes.audioReady) {
                nes.audioReady = false;
                std::lock_guard<std::mutex> lock(audioMutex);
                if ((int)audioBuffer.size() < AUDIO_BUFFER_SIZE * 4)
                    audioBuffer.push_back(nes.audioSample);
            }
        } while (!nes.picture->frame_complete);
        nes.picture->frame_complete = false;

        SDL_UpdateTexture(tex, nullptr, nes.picture->framebuffer.data(), 256 * sizeof(uint32_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);  // nullptr dst = fill logical viewport
        SDL_RenderPresent(ren);
    }

    SDL_PauseAudioDevice(audioDevice, 1);
    SDL_DestroyTexture(tex);

    // Clear audio buffer between games
    { std::lock_guard<std::mutex> l(audioMutex); audioBuffer.clear(); }

    // Reset logical size so GUI renders at window resolution
    SDL_RenderSetLogicalSize(ren, 0, 0);
    return backToMenu;   // true → go back to launcher, false → quit
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    Config cfg = Config::load();

    // ── Audio setup ──────────────────────────────────────────────────────────
    SDL_AudioSpec want{}, have{};
    want.freq     = 44100;
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = AUDIO_BUFFER_SIZE;
    want.callback = audioCallback;
    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (audioDevice == 0)
        fprintf(stderr, "SDL_OpenAudioDevice: %s\n", SDL_GetError());

    // ── Window (starts GUI-sized; resized when a game is launched) ───────────
    SDL_Window* win = SDL_CreateWindow("NESPC \xe2\x80\x93 Launcher",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win) { fprintf(stderr, "Window: %s\n", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    // ── If a ROM was passed on the CLI, skip the launcher ───────────────────
    if (argc >= 2) {
        std::string romPath = argv[1];
        bool again = runEmulator(win, ren, audioDevice, romPath, cfg);
        if (!again) {
            SDL_CloseAudioDevice(audioDevice);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            SDL_Quit();
            return 0;
        }
    }

    // ── Main launcher loop ───────────────────────────────────────────────────
    while (true) {
        // Show launcher GUI
        std::string romPath = GUI::run(win, ren, cfg);
        if (romPath.empty()) break;   // user closed window

        // Launch selected game; returns true if user pressed ESC → back to menu
        bool backToMenu = runEmulator(win, ren, audioDevice, romPath, cfg);
        if (!backToMenu) break;

        // Back to launcher: restore GUI window dimensions
        SDL_SetWindowFullscreen(win, 0);
        SDL_SetWindowSize(win, 1280, 800);
        SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_SetWindowTitle(win, "NESPC \xe2\x80\x93 Launcher");
    }

    if (audioDevice) SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}