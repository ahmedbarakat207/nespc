#pragma once
#include <cstdint>
#include <memory>
#include <array>
#include "rom.h"

class ppu {
public:
    ppu();
    ~ppu();

    void connect_cartridge(const std::shared_ptr<rom>& cart);
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void clock();
    void reset();
    bool nmi            = false;
    bool frame_complete = false;
    std::array<uint32_t, 256 * 240> framebuffer{};
    union PPUCTRL {
        struct {
            uint8_t nametable_x        : 1;
            uint8_t nametable_y        : 1;
            uint8_t increment_mode     : 1;
            uint8_t pattern_sprite     : 1;
            uint8_t pattern_background : 1;
            uint8_t sprite_size        : 1;
            uint8_t slave_mode         : 1;
            uint8_t enable_nmi         : 1;
        };
        uint8_t reg = 0;
    } control;

    union PPUMASK {
        struct {
            uint8_t grayscale              : 1;
            uint8_t render_background_left : 1;
            uint8_t render_sprites_left    : 1;
            uint8_t render_background      : 1;
            uint8_t render_sprites         : 1;
            uint8_t enhance_red            : 1;
            uint8_t enhance_green          : 1;
            uint8_t enhance_blue           : 1;
        };
        uint8_t reg = 0;
    } mask;

    union PPUSTATUS {
        struct {
            uint8_t unused         : 5;
            uint8_t sprite_overflow: 1;
            uint8_t sprite_zero_hit: 1;
            uint8_t vertical_blank : 1;
        };
        uint8_t reg = 0;
    } status;

    int16_t scanline = 0;
    int16_t cycle    = 0;
    struct OAMEntry {
        uint8_t y;
        uint8_t id;
        uint8_t attribute;
        uint8_t x;
    };
    std::array<uint8_t, 256> oam{};   // flat byte view
    OAMEntry OAM[64]{};                // structured view
    union loopy_register {
        struct {
            uint16_t coarse_x    : 5;
            uint16_t coarse_y    : 5;
            uint16_t nametable_x : 1;
            uint16_t nametable_y : 1;
            uint16_t fine_y      : 3;
            uint16_t unused      : 1;
        };
        uint16_t reg = 0x0000;
    };
    loopy_register vram_addr;
    loopy_register tram_addr;

private:
    std::shared_ptr<rom> cartridge = nullptr;

    uint8_t tblName[2][1024]{};
    uint8_t tblPalette[32]{};
    uint8_t tblPattern[2][4096]{};

    uint8_t fine_x          = 0x00;
    uint8_t address_latch   = 0x00;
    uint8_t ppu_data_buffer = 0x00;
    uint8_t oam_addr        = 0x00;

    // Background shifters
    uint8_t  bg_next_tile_id     = 0x00;
    uint8_t  bg_next_tile_attrib = 0x00;
    uint8_t  bg_next_tile_lsb   = 0x00;
    uint8_t  bg_next_tile_msb   = 0x00;
    uint16_t bg_shifter_pattern_lo = 0;
    uint16_t bg_shifter_pattern_hi = 0;
    uint16_t bg_shifter_attrib_lo  = 0;
    uint16_t bg_shifter_attrib_hi  = 0;

    // Sprite rendering
    OAMEntry spriteScanline[8]{};
    uint8_t  sprite_count = 0;
    uint8_t  sprite_shifter_pattern_lo[8]{};
    uint8_t  sprite_shifter_pattern_hi[8]{};
    bool     bSpriteZeroHitPossible    = false;
    bool     bSpriteZeroBeingRendered  = false;

    // NES palette
    static constexpr uint32_t nes_palette[64] = {
        0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
        0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
        0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
        0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082, 0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
        0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
        0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC, 0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000
    };

    uint8_t  ppuRead (uint16_t addr, bool rdonly = false);
    void     ppuWrite(uint16_t addr, uint8_t data);
    uint32_t GetColourFromPaletteRam(uint8_t palette, uint8_t pixel);
};
