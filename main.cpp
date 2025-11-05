#include <iostream>
#include "bus.h"
#include "6502.h"
#include <SDL2/SDL.h>
#include <stdio.h>

using namespace std;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom_file>\n", argv[0]);
        return 1;
    }

    printf("Starting NES Emulator...\n");
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    printf("SDL initialized successfully.\n");

    // Create window
    SDL_Window *win = SDL_CreateWindow("NESPC Emulator", 
                                      SDL_WINDOWPOS_CENTERED, 
                                      SDL_WINDOWPOS_CENTERED, 
                                      256 * 2, 240 * 2, 
                                      SDL_WINDOW_SHOWN);
    if (win == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    printf("Window created successfully.\n");

    // Create renderer
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, 
                                          SDL_RENDERER_ACCELERATED | 
                                          SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr) {
        SDL_DestroyWindow(win);
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    printf("Renderer created successfully.\n");

    // Create texture
    SDL_Texture *texture = SDL_CreateTexture(ren,
                                            SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            256, 240);
    if (texture == nullptr) {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    printf("Texture created successfully.\n");

    // Initialize NES system
    printf("Initializing NES system...\n");
    bus nes;
    printf("Bus created.\n");
    
    try {
        printf("Loading ROM: %s\n", argv[1]);
        auto cart = std::make_shared<rom>(argv[1]);
        printf("ROM loaded successfully.\n");
        
        printf("Inserting cartridge...\n");
        nes.insert_cartridge(cart);
        printf("Cartridge inserted.\n");
        
        printf("Resetting NES...\n");
        nes.reset();
        printf("NES reset complete.\n");

        // FORCE PPU INITIALIZATION - Super Mario Bros. expects this
        printf("Forcing PPU initialization...\n");
        nes.ppu->control.reg = 0x90; // Enable NMI, use pattern table 0 for background
        nes.ppu->mask.reg = 0x1E;    // Show background and sprites

        // Initialize framebuffer with visible pattern
        for (int i = 0; i < 256 * 240; i++) {
            int x = i % 256;
            int y = i / 256;
            
            // Create a visible test pattern
            if ((x / 32 + y / 30) % 2 == 0) {
                nes.ppu->framebuffer[i] = 0xFFFF0000; // Red
            } else {
                nes.ppu->framebuffer[i] = 0xFF0000FF; // Blue
            }
            
            // Add grid lines
            if (x % 32 == 0 || y % 30 == 0) {
                nes.ppu->framebuffer[i] = 0xFF00FF00; // Green
            }
        }

        printf("PPU initialization complete - Control: %02X, Mask: %02X\n", 
               nes.ppu->control.reg, nes.ppu->mask.reg);
    } catch (const std::exception& e) {
        printf("Error during initialization: %s\n", e.what());
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;
    int frame_count = 0;
    
    printf("Entering main loop...\n");
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        try {
            // Run until frame is complete
            do {
                nes.clock();
            } while (!nes.ppu->frame_complete);

            nes.ppu->frame_complete = false;
            frame_count++;

            // Update texture with framebuffer
            SDL_UpdateTexture(texture, NULL, nes.ppu->framebuffer.data(), 256 * sizeof(uint32_t));
            
            // Clear screen and render
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            
            SDL_Rect dstRect = {0, 0, 256 * 2, 240 * 2};
            SDL_RenderCopy(ren, texture, NULL, &dstRect);
            
            SDL_RenderPresent(ren);

            // Simplified debug output
            if (frame_count % 60 == 0) {
                printf("=== Frame %d ===\n", frame_count);
                printf("PPU Control: %02X (NMI:%d, BG:%d, SPR:%d)\n", 
                       nes.ppu->control.reg, 
                       (nes.ppu->control.reg & 0x80) >> 7,
                       (nes.ppu->control.reg & 0x10) >> 4,
                       (nes.ppu->control.reg & 0x08) >> 3);
                printf("PPU Mask: %02X (ShowBG:%d, ShowSPR:%d)\n",
                       nes.ppu->mask.reg,
                       (nes.ppu->mask.reg & 0x08) >> 3,
                       (nes.ppu->mask.reg & 0x10) >> 4);
                printf("PPU Status: %02X (VBlank:%d)\n",
                       nes.ppu->status.reg,
                       (nes.ppu->status.reg & 0x80) >> 7);
                printf("VRAM: %04X, Scanline: %d, Cycle: %d\n",
                       nes.ppu->vram_addr, nes.ppu->scanline, nes.ppu->cycle);
                printf("CPU PC: %04X, A: %02X, X: %02X, Y: %02X\n",
                       nes.cpu.pc, nes.cpu.a, nes.cpu.x, nes.cpu.y);
                
                // Check first few OAM bytes
                printf("First 8 OAM bytes: ");
                for (int i = 0; i < 8; i++) {
                    printf("%02X ", nes.ppu->oam[i]);
                }
                printf("\n");
                printf("================\n");
            }
            if ((nes.ppu->mask.reg & 0x18) != 0x18) { // Check if both bit 3 and 4 are not set
    static bool forced_rendering = false;
    if (!forced_rendering) {
        printf("=== FORCING FULL RENDERING ===\n");
        printf("Current mask: %02X, forcing to 1E\n", nes.ppu->mask.reg);
        
        // Write to PPU mask register to enable full rendering
        // 0x1E = 00011110 - show background, sprites, and leftmost 8 pixels
        nes.cpuWrite(0x2001, 0x1E);
        
        forced_rendering = true;
    }
}
            // In the main loop, add this check:
if (frame_count % 30 == 0) {
    // Check if PPU is actually producing pixels
    int colored_pixels = 0;
    for (int i = 0; i < 256 * 240; i++) {
        if (nes.ppu->framebuffer[i] != 0xFF000000) {
            colored_pixels++;
        }
    }
    printf("Frame %d: %d non-black pixels in framebuffer\n", frame_count, colored_pixels);
    
    // Check a few specific pixels
    printf("Sample pixels: [100,100]=%08X, [150,100]=%08X\n", 
           nes.ppu->framebuffer[100 * 256 + 100],
           nes.ppu->framebuffer[100 * 256 + 150]);
}
            
        } catch (const std::exception& e) {
            printf("Error during emulation: %s\n", e.what());
            running = false;
        }
    }

    printf("Shutting down...\n");
    
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    
    printf("Emulator stopped.\n");
    return 0;
}