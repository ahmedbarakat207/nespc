#include "ppu.h"
#include "rom.h"
#include <cstring>
#include <cstdio>

ppu::ppu() {
    memset(tblName,    0, sizeof(tblName));
    memset(tblPalette, 0, sizeof(tblPalette));
    memset(tblPattern, 0, sizeof(tblPattern));
    memset(OAM,        0, sizeof(OAM));
    memset(spriteScanline, 0, sizeof(spriteScanline));
    memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
    memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
    framebuffer.fill(0xFF000000);
    oam.fill(0);
}

ppu::~ppu() {}

void ppu::reset() {
    control.reg    = 0;
    mask.reg       = 0;
    status.reg     = 0;
    fine_x         = 0;
    address_latch  = 0;
    ppu_data_buffer= 0;
    scanline       = 0;
    cycle          = 0;
    oam_addr       = 0;
    vram_addr.reg  = 0;
    tram_addr.reg  = 0;
    bg_next_tile_id     = 0;
    bg_next_tile_attrib = 0;
    bg_next_tile_lsb    = 0;
    bg_next_tile_msb    = 0;
    bg_shifter_pattern_lo = 0;
    bg_shifter_pattern_hi = 0;
    bg_shifter_attrib_lo  = 0;
    bg_shifter_attrib_hi  = 0;
    sprite_count          = 0;
    bSpriteZeroHitPossible   = false;
    bSpriteZeroBeingRendered = false;
    nmi            = false;
    frame_complete = false;
    memset(tblName,    0, sizeof(tblName));
    memset(tblPalette, 0, sizeof(tblPalette));
    memset(OAM,        0, sizeof(OAM));
    memset(spriteScanline, 0, sizeof(spriteScanline));
    memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
    memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
    framebuffer.fill(0xFF000000);
    oam.fill(0);
}

void ppu::connect_cartridge(const std::shared_ptr<rom>& cart) {
    cartridge = cart;
}

void ppu::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr & 0x07) {
        case 0: 
            control.reg = data;
            tram_addr.nametable_x = control.nametable_x;
            tram_addr.nametable_y = control.nametable_y;
            break;
        case 1: 
            mask.reg = data;
            break;
        case 3: 
            oam_addr = data;
            break;
        case 4: 
            oam[oam_addr] = data;
            {
                uint8_t si = oam_addr / 4;
                switch (oam_addr % 4) {
                    case 0: OAM[si].y         = data; break;
                    case 1: OAM[si].id        = data; break;
                    case 2: OAM[si].attribute = data; break;
                    case 3: OAM[si].x         = data; break;
                }
            }
            oam_addr++;
            break;
        case 5: 
            if (address_latch == 0) {
                fine_x = data & 0x07;
                tram_addr.coarse_x = data >> 3;
                address_latch = 1;
            } else {
                tram_addr.fine_y   = data & 0x07;
                tram_addr.coarse_y = data >> 3;
                address_latch = 0;
            }
            break;
        case 6: 
            if (address_latch == 0) {
                tram_addr.reg = (uint16_t)((data & 0x3F) << 8) | (tram_addr.reg & 0x00FF);
                address_latch = 1;
            } else {
                tram_addr.reg = (tram_addr.reg & 0xFF00) | data;
                vram_addr = tram_addr;
                address_latch = 0;
            }
            break;
        case 7: 
            ppuWrite(vram_addr.reg, data);
            vram_addr.reg += (control.increment_mode ? 32 : 1);
            break;
    }
}

uint8_t ppu::cpuRead(uint16_t addr) {
    uint8_t data = 0x00;
    switch (addr & 0x07) {
        case 2: 
            data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);
            status.vertical_blank = 0;
            address_latch = 0;
            break;
        case 4: 
            data = oam[oam_addr];
            break;
        case 7: 
            data = ppu_data_buffer;
            ppu_data_buffer = ppuRead(vram_addr.reg);
            if (vram_addr.reg >= 0x3F00) data = ppu_data_buffer;
            vram_addr.reg += (control.increment_mode ? 32 : 1);
            break;
    }
    return data;
}

uint8_t ppu::ppuRead(uint16_t addr, bool rdonly) {
    (void)rdonly;
    uint8_t data = 0x00;
    addr &= 0x3FFF;

    if (cartridge && cartridge->ppuRead(addr, data))
        return data;

    if (addr <= 0x1FFF) {
        
        return tblPattern[(addr >> 12) & 1][addr & 0x0FFF];
    }

    if (addr <= 0x3EFF) {
        addr &= 0x0FFF;
        if (cartridge) {
            if (cartridge->mirror == rom::MIRROR::VERTICAL) {
                return tblName[(addr >> 10) & 0x01][addr & 0x03FF];
            } else if (cartridge->mirror == rom::MIRROR::HORIZONTAL) {
                return tblName[(addr >> 11) & 0x01][addr & 0x03FF];
            } else if (cartridge->mirror == rom::MIRROR::ONESCREEN_LO) {
                return tblName[0][addr & 0x03FF];
            } else if (cartridge->mirror == rom::MIRROR::ONESCREEN_HI) {
                return tblName[1][addr & 0x03FF];
            }
        }
        return tblName[0][addr & 0x03FF];
    }

    
    addr &= 0x001F;
    if (addr == 0x10) addr = 0x00;
    if (addr == 0x14) addr = 0x04;
    if (addr == 0x18) addr = 0x08;
    if (addr == 0x1C) addr = 0x0C;
    return tblPalette[addr] & (mask.grayscale ? 0x30 : 0x3F);
}

void ppu::ppuWrite(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;

    if (cartridge && cartridge->ppuWrite(addr, data))
        return;

    if (addr <= 0x1FFF) {
        tblPattern[(addr >> 12) & 1][addr & 0x0FFF] = data;
        return;
    }

    if (addr <= 0x3EFF) {
        addr &= 0x0FFF;
        if (cartridge) {
            if (cartridge->mirror == rom::MIRROR::VERTICAL) {
                tblName[(addr >> 10) & 0x01][addr & 0x03FF] = data;
            } else if (cartridge->mirror == rom::MIRROR::HORIZONTAL) {
                tblName[(addr >> 11) & 0x01][addr & 0x03FF] = data;
            } else if (cartridge->mirror == rom::MIRROR::ONESCREEN_LO) {
                tblName[0][addr & 0x03FF] = data;
            } else if (cartridge->mirror == rom::MIRROR::ONESCREEN_HI) {
                tblName[1][addr & 0x03FF] = data;
            }
            return;
        }
        tblName[0][addr & 0x03FF] = data;
        return;
    }

    
    addr &= 0x001F;
    if (addr == 0x10) addr = 0x00;
    if (addr == 0x14) addr = 0x04;
    if (addr == 0x18) addr = 0x08;
    if (addr == 0x1C) addr = 0x0C;
    tblPalette[addr] = data;
}

uint32_t ppu::GetColourFromPaletteRam(uint8_t palette, uint8_t pixel) {
    uint8_t idx = ppuRead(0x3F00 + ((palette & 0x07) << 2) + (pixel & 0x03)) & 0x3F;
    return nes_palette[idx];
}

void ppu::clock() {

    auto IncrementScrollX = [&]() {
        if (!mask.render_background && !mask.render_sprites) return;
        if (vram_addr.coarse_x == 31) {
            vram_addr.coarse_x    = 0;
            vram_addr.nametable_x = ~vram_addr.nametable_x;
        } else {
            vram_addr.coarse_x++;
        }
    };

    auto IncrementScrollY = [&]() {
        if (!mask.render_background && !mask.render_sprites) return;
        if (vram_addr.fine_y < 7) {
            vram_addr.fine_y++;
        } else {
            vram_addr.fine_y = 0;
            if (vram_addr.coarse_y == 29) {
                vram_addr.coarse_y    = 0;
                vram_addr.nametable_y = ~vram_addr.nametable_y;
            } else if (vram_addr.coarse_y == 31) {
                vram_addr.coarse_y = 0;
            } else {
                vram_addr.coarse_y++;
            }
        }
    };

    auto TransferAddressX = [&]() {
        if (!mask.render_background && !mask.render_sprites) return;
        vram_addr.nametable_x = tram_addr.nametable_x;
        vram_addr.coarse_x    = tram_addr.coarse_x;
    };

    auto TransferAddressY = [&]() {
        if (!mask.render_background && !mask.render_sprites) return;
        vram_addr.fine_y      = tram_addr.fine_y;
        vram_addr.nametable_y = tram_addr.nametable_y;
        vram_addr.coarse_y    = tram_addr.coarse_y;
    };

    auto LoadBackgroundShifters = [&]() {
        bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
        bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;
        bg_shifter_attrib_lo  = (bg_shifter_attrib_lo  & 0xFF00) | ((bg_next_tile_attrib & 0x01) ? 0xFF : 0x00);
        bg_shifter_attrib_hi  = (bg_shifter_attrib_hi  & 0xFF00) | ((bg_next_tile_attrib & 0x02) ? 0xFF : 0x00);
    };

    auto UpdateShifters = [&]() {
        if (mask.render_background) {
            bg_shifter_pattern_lo <<= 1;
            bg_shifter_pattern_hi <<= 1;
            bg_shifter_attrib_lo  <<= 1;
            bg_shifter_attrib_hi  <<= 1;
        }
        if (mask.render_sprites && cycle >= 1 && cycle < 258) {
            for (int i = 0; i < sprite_count; i++) {
                if (spriteScanline[i].x > 0) {
                    spriteScanline[i].x--;
                } else {
                    sprite_shifter_pattern_lo[i] <<= 1;
                    sprite_shifter_pattern_hi[i] <<= 1;
                }
            }
        }
    };

    
    if (scanline >= -1 && scanline < 240) {

        if (scanline == 0 && cycle == 0)
            cycle = 1; 

        if (scanline == -1 && cycle == 1) {
            status.vertical_blank  = 0;
            status.sprite_overflow = 0;
            status.sprite_zero_hit = 0;
            memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
            memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
        }

        if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338)) {
            UpdateShifters();
            switch ((cycle - 1) & 7) {
                case 0:
                    LoadBackgroundShifters();
                    bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
                    break;
                case 2:
                    bg_next_tile_attrib = ppuRead(
                        0x23C0
                        | (vram_addr.nametable_y << 11)
                        | (vram_addr.nametable_x << 10)
                        | ((vram_addr.coarse_y >> 2) << 3)
                        |  (vram_addr.coarse_x >> 2));
                    if (vram_addr.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
                    if (vram_addr.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
                    bg_next_tile_attrib &= 0x03;
                    break;
                case 4:
                    bg_next_tile_lsb = ppuRead(
                        ((uint16_t)control.pattern_background << 12)
                        + ((uint16_t)bg_next_tile_id << 4)
                        + vram_addr.fine_y);
                    break;
                case 6:
                    bg_next_tile_msb = ppuRead(
                        ((uint16_t)control.pattern_background << 12)
                        + ((uint16_t)bg_next_tile_id << 4)
                        + vram_addr.fine_y + 8);
                    break;
                case 7:
                    IncrementScrollX();
                    break;
            }
        }

        if (cycle == 256) IncrementScrollY();
        if (cycle == 257) { LoadBackgroundShifters(); TransferAddressX(); }
        if (cycle == 260 && scanline >= 0 && scanline < 240 && (mask.render_background || mask.render_sprites)) {
            if (cartridge && cartridge->pMapper) {
                cartridge->pMapper->scanline();
            }
        }
        if (cycle == 338 || cycle == 340)
            bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
        if (scanline == -1 && cycle >= 280 && cycle < 305)
            TransferAddressY();

        
        if (cycle == 257 && scanline >= 0) {
            memset(spriteScanline, 0xFF, sizeof(spriteScanline));
            sprite_count = 0;
            bSpriteZeroHitPossible = false;

            for (uint8_t e = 0; e < 64; e++) {
                
                if (OAM[e].y >= 0xEF) continue;
                
                int16_t diff = (int16_t)scanline - (int16_t)(OAM[e].y + 1);
                int16_t height = control.sprite_size ? 16 : 8;
                if (diff >= 0 && diff < height) {
                    if (sprite_count < 8) {
                        if (e == 0) bSpriteZeroHitPossible = true;
                        spriteScanline[sprite_count++] = OAM[e];
                    } else {
                        status.sprite_overflow = 1;
                        break;
                    }
                }
            }
        }

        
        if (cycle == 340) {
            for (uint8_t i = 0; i < sprite_count; i++) {
                uint8_t  sp_lo, sp_hi;
                uint16_t addr_lo;

                int16_t row = (int16_t)scanline - (int16_t)(spriteScanline[i].y + 1);

                if (!control.sprite_size) {
                    
                    if (spriteScanline[i].attribute & 0x80) row = 7 - row; 
                    addr_lo = ((uint16_t)control.pattern_sprite << 12)
                            | ((uint16_t)spriteScanline[i].id  <<  4)
                            | (uint16_t)row;
                } else {
                    
                    if (spriteScanline[i].attribute & 0x80) row = 15 - row; 
                    uint8_t tile_bank = spriteScanline[i].id & 0x01;
                    uint8_t tile_id   = spriteScanline[i].id & 0xFE;
                    if (row >= 8) { tile_id++; row -= 8; }
                    addr_lo = ((uint16_t)tile_bank << 12)
                            | ((uint16_t)tile_id   <<  4)
                            | (uint16_t)row;
                }

                sp_lo = ppuRead(addr_lo);
                sp_hi = ppuRead(addr_lo + 8);

                
                if (spriteScanline[i].attribute & 0x40) {
                    auto flip = [](uint8_t b) -> uint8_t {
                        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                        return b;
                    };
                    sp_lo = flip(sp_lo);
                    sp_hi = flip(sp_hi);
                }

                sprite_shifter_pattern_lo[i] = sp_lo;
                sprite_shifter_pattern_hi[i] = sp_hi;
            }
        }
    }

    
    
    
    
    if (scanline == 241 && cycle == 1) {
        status.vertical_blank = 1;
    }
    if (scanline == 241 && cycle == 2) {
        if (control.enable_nmi) nmi = true;
    }

    
    uint8_t bg_pixel = 0, bg_palette = 0;
    uint8_t fg_pixel = 0, fg_palette = 0, fg_priority = 0;

    if (mask.render_background) {
        uint16_t mux = 0x8000 >> fine_x;
        uint8_t p0   = (bg_shifter_pattern_lo & mux) ? 1 : 0;
        uint8_t p1   = (bg_shifter_pattern_hi & mux) ? 1 : 0;
        bg_pixel   = (p1 << 1) | p0;
        uint8_t a0 = (bg_shifter_attrib_lo & mux) ? 1 : 0;
        uint8_t a1 = (bg_shifter_attrib_hi & mux) ? 1 : 0;
        bg_palette = (a1 << 1) | a0;
    }

    if (mask.render_sprites) {
        bSpriteZeroBeingRendered = false;
        for (uint8_t i = 0; i < sprite_count; i++) {
            if (spriteScanline[i].x == 0) {
                uint8_t lo = (sprite_shifter_pattern_lo[i] & 0x80) ? 1 : 0;
                uint8_t hi = (sprite_shifter_pattern_hi[i] & 0x80) ? 1 : 0;
                fg_pixel    = (hi << 1) | lo;
                fg_palette  = (spriteScanline[i].attribute & 0x03) + 0x04;
                fg_priority = (spriteScanline[i].attribute & 0x20) ? 0 : 1;
                if (fg_pixel != 0) {
                    if (i == 0) bSpriteZeroBeingRendered = true;
                    break;
                }
            }
        }
    }

    uint8_t pixel = 0, palette = 0;
    if      (bg_pixel == 0 && fg_pixel == 0) { pixel = 0; palette = 0; }
    else if (bg_pixel == 0)                  { pixel = fg_pixel;  palette = fg_palette; }
    else if (fg_pixel == 0)                  { pixel = bg_pixel;  palette = bg_palette; }
    else {
        if (fg_priority) { pixel = fg_pixel; palette = fg_palette; }
        else             { pixel = bg_pixel; palette = bg_palette; }
    }

    if (bSpriteZeroHitPossible && bSpriteZeroBeingRendered) {
        if (mask.render_background && mask.render_sprites) {
            if (!(mask.render_background_left && mask.render_sprites_left)) {
                if (cycle >= 9 && cycle < 258) {
                    status.sprite_zero_hit = 1;
                }
            } else {
                if (cycle >= 1 && cycle < 258) {
                    status.sprite_zero_hit = 1;
                }
            }
        }
    }

    
    if (scanline >= 0 && scanline < 240 && cycle >= 1 && cycle <= 256) {
        framebuffer[(uint16_t)scanline * 256 + (uint16_t)(cycle - 1)] =
            GetColourFromPaletteRam(palette, pixel);
    }

    
    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 261) {
            scanline       = -1;
            frame_complete = true;
        }
    }
}
