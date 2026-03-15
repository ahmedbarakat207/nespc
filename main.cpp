#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>
#include "bus.h"
#include <SDL2/SDL.h>

static std::vector<float> audioBuffer;
static std::mutex         audioMutex;
static const int          AUDIO_BUFFER_SIZE = 2048;

static void audioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    float* out    = reinterpret_cast<float*>(stream);
    int    frames = len / sizeof(float);

    std::lock_guard<std::mutex> lock(audioMutex);
    int available = (int)audioBuffer.size();

    for (int i = 0; i < frames; i++) {
        out[i] = (i < available) ? audioBuffer[i] : 0.0f;
    }
    if (available >= frames)
        audioBuffer.erase(audioBuffer.begin(), audioBuffer.begin() + frames);
    else
        audioBuffer.clear();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom_file>\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("NESPC Emulator",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       256 * 2, 240 * 2,
                                       SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "Window: %s\n", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                            SDL_RENDERER_ACCELERATED |
                                            SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    SDL_Texture *tex = SDL_CreateTexture(ren,
                                         SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         256, 240);
    if (!tex) { fprintf(stderr, "Texture: %s\n", SDL_GetError()); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    SDL_AudioSpec want{}, have{};
    want.freq     = 44100;
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = AUDIO_BUFFER_SIZE;
    want.callback = audioCallback;

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (audioDevice == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(audioDevice, 0);

    bus nes;

    try {
        auto cart = std::make_shared<rom>(argv[1]);
        nes.insert_cartridge(cart);
        nes.reset();
    } catch (const std::exception &e) {
        fprintf(stderr, "ROM Error: %s\n", e.what());
        SDL_CloseAudioDevice(audioDevice);
        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_A:      nes.controller[0] |= 0x80; break;
                    case SDL_SCANCODE_S:      nes.controller[0] |= 0x40; break;
                    case SDL_SCANCODE_RSHIFT: nes.controller[0] |= 0x20; break;
                    case SDL_SCANCODE_RETURN: nes.controller[0] |= 0x10; break;
                    case SDL_SCANCODE_UP:     nes.controller[0] |= 0x08; break;
                    case SDL_SCANCODE_DOWN:   nes.controller[0] |= 0x04; break;
                    case SDL_SCANCODE_LEFT:   nes.controller[0] |= 0x02; break;
                    case SDL_SCANCODE_RIGHT:  nes.controller[0] |= 0x01; break;
                    case SDL_SCANCODE_R:      nes.reset(); break;
                    case SDL_SCANCODE_ESCAPE: running = false; break;
                    default: break;
                }
            }
            if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_A:      nes.controller[0] &= ~0x80; break;
                    case SDL_SCANCODE_S:      nes.controller[0] &= ~0x40; break;
                    case SDL_SCANCODE_RSHIFT: nes.controller[0] &= ~0x20; break;
                    case SDL_SCANCODE_RETURN: nes.controller[0] &= ~0x10; break;
                    case SDL_SCANCODE_UP:     nes.controller[0] &= ~0x08; break;
                    case SDL_SCANCODE_DOWN:   nes.controller[0] &= ~0x04; break;
                    case SDL_SCANCODE_LEFT:   nes.controller[0] &= ~0x02; break;
                    case SDL_SCANCODE_RIGHT:  nes.controller[0] &= ~0x01; break;
                    default: break;
                }
            }
        }

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
        SDL_Rect dst = {0, 0, 256 * 2, 240 * 2};
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_RenderPresent(ren);
    }

    SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
