#include "ppu.h"
#include "rom.h"
#include <iostream>
#include <cstring>

ppu::ppu() {
    printf("PPU: Constructor started\n");
    
    // Initialize the shared_ptr first
    cartridge = nullptr;
    
    // Initialize arrays with memset - use sizeof for safety
    memset(tblName, 0, sizeof(tblName));
    memset(tblPalette, 0, sizeof(tblPalette));
    memset(tblPattern, 0, sizeof(tblPattern));
    memset(OAM, 0, sizeof(OAM));
    memset(spriteScanline, 0, sizeof(spriteScanline));
    memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
    memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
    
    // Initialize framebuffer
    framebuffer.fill(0xFF000000);
    
    // Initialize other members
    control.reg = 0;
    mask.reg = 0;
    status.reg = 0;
    fine_x = 0x00;
    address_latch = 0x00;
    ppu_data_buffer = 0x00;
    scanline = 0;
    cycle = 0;
    oam_addr = 0x00;
    
    vram_addr.reg = 0x0000;
    tram_addr.reg = 0x0000;
    
    printf("PPU: Constructor completed\n");
}
ppu::~ppu() {
    printf("PPU: Destructor called\n");
}

void ppu::reset() {
    printf("PPU: Reset started\n");
    
    control.reg = 0;
    mask.reg = 0;
    status.reg = 0;
    fine_x = 0x00;
    address_latch = 0x00;
    ppu_data_buffer = 0x00;
    scanline = 0;
    cycle = 0;
    oam_addr = 0x00;
    
    vram_addr.reg = 0x0000;
    tram_addr.reg = 0x0000;
    
    // Clear memory arrays
    memset(tblName, 0, sizeof(tblName));
    memset(tblPalette, 0, sizeof(tblPalette));
    memset(OAM, 0, sizeof(OAM));
    memset(spriteScanline, 0, sizeof(spriteScanline));
    memset(sprite_shifter_pattern_lo, 0, sizeof(sprite_shifter_pattern_lo));
    memset(sprite_shifter_pattern_hi, 0, sizeof(sprite_shifter_pattern_hi));
    
    // Reset background rendering
    bg_next_tile_id = 0x00;
    bg_next_tile_attrib = 0x00;
    bg_next_tile_lsb = 0x00;
    bg_next_tile_msb = 0x00;
    bg_shifter_pattern_lo = 0x0000;
    bg_shifter_pattern_hi = 0x0000;
    bg_shifter_attrib_lo = 0x0000;
    bg_shifter_attrib_hi = 0x0000;
    
    // Reset sprites
    sprite_count = 0;
    bSpriteZeroHitPossible = false;
    bSpriteZeroBeingRendered = false;
    
    // Initialize framebuffer
    framebuffer.fill(0xFF000000);
    
    printf("PPU: Reset completed\n");
}

void ppu::connect_cartridge(const std::shared_ptr<rom>& cart) {
    printf("PPU: Connecting cartridge\n");
    if (!cart) {
        printf("PPU: WARNING - Null cartridge passed!\n");
        return;
    }
    cartridge = cart;
    printf("PPU: Cartridge connected - PRG: %zu bytes, CHR: %zu bytes\n", 
           cart->prgMemory.size(), cart->chrMemory.size());
}

void ppu::cpuWrite(uint16_t addr, uint8_t data) {
    //printf("PPU CPU Write: addr=%04X, data=%02X\n", addr, data);
    
    addr &= 0x0007;

    switch (addr) {
        case 0x0000: // Control
            control.reg = data;
            tram_addr.nametable_x = control.nametable_x;
            tram_addr.nametable_y = control.nametable_y;
            break;
            
        case 0x0001: // Mask
            mask.reg = data;
            break;
            
        case 0x0002: // Status
            break;
            
        case 0x0003: // OAM Address
            oam_addr = data;
            break;
            
        case 0x0004: // OAM Data
        {
            // Write to both the linear OAM array and structured OAM
            oam[oam_addr] = data;
            
            // Update structured OAM
            uint8_t sprite_idx = oam_addr / 4;
            uint8_t field = oam_addr % 4;
            switch (field) {
                case 0: OAM[sprite_idx].y = data; break;
                case 1: OAM[sprite_idx].id = data; break;
                case 2: OAM[sprite_idx].attribute = data; break;
                case 3: OAM[sprite_idx].x = data; break;
            }
            
            oam_addr++;
            break;
        }
            
        case 0x0005: // Scroll
            if (address_latch == 0) {
                fine_x = data & 0x07;
                tram_addr.coarse_x = data >> 3;
                address_latch = 1;
            } else {
                tram_addr.fine_y = data & 0x07;
                tram_addr.coarse_y = data >> 3;
                address_latch = 0;
            }
            break;
            
        case 0x0006: // PPU Address
            if (address_latch == 0) {
                tram_addr.reg = (uint16_t)((data & 0x3F) << 8) | (tram_addr.reg & 0x00FF);
                address_latch = 1;
            } else {
                tram_addr.reg = (tram_addr.reg & 0xFF00) | data;
                vram_addr = tram_addr;
                address_latch = 0;
            }
            break;
            
        case 0x0007: // PPU Data
            ppuWrite(vram_addr.reg, data);
            vram_addr.reg += (control.increment_mode ? 32 : 1);
            break;
    }
}

uint8_t ppu::cpuRead(uint16_t addr) {
    uint8_t data = 0x00;
    addr &= 0x0007;

    switch (addr) {
        case 0x0000: // Control - Write only
            break;
            
        case 0x0001: // Mask - Write only
            break;
            
        case 0x0002: // Status
            data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);
            status.vertical_blank = 0;
            address_latch = 0;
            break;
            
        case 0x0003: // OAM Address - Write only
            break;
            
        case 0x0004: // OAM Data
            data = oam[oam_addr];
            break;
            
        case 0x0005: // Scroll - Write only
            break;
            
        case 0x0006: // Address - Write only
            break;
            
        case 0x0007: // Data
            data = ppu_data_buffer;
            ppu_data_buffer = ppuRead(vram_addr.reg);
            if (vram_addr.reg >= 0x3F00) data = ppu_data_buffer;
            vram_addr.reg += (control.increment_mode ? 32 : 1);
            break;
    }

    return data;
}

uint8_t ppu::ppuRead(uint16_t addr, bool rdonly) {
    (void)rdonly; // Silence unused parameter warning
    
    uint8_t data = 0x00;
    addr &= 0x3FFF;

    if (cartridge && cartridge->ppuRead(addr, data)) {
        return data;
    }
    
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Pattern tables - safe access
        uint8_t table = (addr & 0x1000) >> 12;
        uint16_t offset = addr & 0x0FFF;
        if (table < 2 && offset < 4096) {
            data = tblPattern[table][offset];
        }
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
        // Name tables
        addr &= 0x0FFF;
        
        uint16_t mirrored_addr = 0;
        if (cartridge && cartridge->mirror == rom::MIRROR::VERTICAL) {
            // Vertical mirroring
            if (addr < 0x0800) {
                mirrored_addr = addr & 0x03FF;
                if (mirrored_addr < 1024) data = tblName[0][mirrored_addr];
            } else if (addr < 0x1000) {
                mirrored_addr = (addr - 0x0800) & 0x03FF;
                if (mirrored_addr < 1024) data = tblName[1][mirrored_addr];
            }
        } else {
            // Horizontal mirroring
            if (addr < 0x0800) {
                mirrored_addr = addr & 0x03FF;
                if (mirrored_addr < 1024) data = tblName[0][mirrored_addr];
            } else {
                mirrored_addr = (addr - 0x0800) & 0x03FF;
                if (mirrored_addr < 1024) data = tblName[1][mirrored_addr];
            }
        }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        // Palette RAM
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        if (addr < 32) {
            data = tblPalette[addr] & (mask.grayscale ? 0x30 : 0x3F);
        }
    }

    return data;
}

void ppu::ppuWrite(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;

    if (cartridge && cartridge->ppuWrite(addr, data)) {
        return;
    }
    
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Pattern tables - safe access
        uint8_t table = (addr & 0x1000) >> 12;
        uint16_t offset = addr & 0x0FFF;
        if (table < 2 && offset < 4096) {
            tblPattern[table][offset] = data;
        }
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
        // Name tables
        addr &= 0x0FFF;
        
        uint16_t mirrored_addr = 0;
        if (cartridge && cartridge->mirror == rom::MIRROR::VERTICAL) {
            // Vertical mirroring
            if (addr < 0x0800) {
                mirrored_addr = addr & 0x03FF;
                if (mirrored_addr < 1024) tblName[0][mirrored_addr] = data;
            } else if (addr < 0x1000) {
                mirrored_addr = (addr - 0x0800) & 0x03FF;
                if (mirrored_addr < 1024) tblName[1][mirrored_addr] = data;
            }
        } else {
            // Horizontal mirroring
            if (addr < 0x0800) {
                mirrored_addr = addr & 0x03FF;
                if (mirrored_addr < 1024) tblName[0][mirrored_addr] = data;
            } else {
                mirrored_addr = (addr - 0x0800) & 0x03FF;
                if (mirrored_addr < 1024) tblName[1][mirrored_addr] = data;
            }
        }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        // Palette RAM
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        if (addr < 32) {
            tblPalette[addr] = data;
        }
    }
}

uint32_t ppu::GetColourFromPaletteRam(uint8_t palette, uint8_t pixel) {
    return nes_palette[ppuRead(0x3F00 + (palette << 2) + pixel, true) & 0x3F];
}

void ppu::clock() {
    auto IncrementScrollX = [&]() {
        if (mask.render_background || mask.render_sprites) {
            if (vram_addr.coarse_x == 31) {
                vram_addr.coarse_x = 0;
                vram_addr.nametable_x = ~vram_addr.nametable_x;
            } else {
                vram_addr.coarse_x++;
            }
        }
    };

    auto IncrementScrollY = [&]() {
        if (mask.render_background || mask.render_sprites) {
            if (vram_addr.fine_y < 7) {
                vram_addr.fine_y++;
            } else {
                vram_addr.fine_y = 0;
                if (vram_addr.coarse_y == 29) {
                    vram_addr.coarse_y = 0;
                    vram_addr.nametable_y = ~vram_addr.nametable_y;
                } else if (vram_addr.coarse_y == 31) {
                    vram_addr.coarse_y = 0;
                } else {
                    vram_addr.coarse_y++;
                }
            }
        }
    };

    auto TransferAddressX = [&]() {
        if (mask.render_background || mask.render_sprites) {
            vram_addr.nametable_x = tram_addr.nametable_x;
            vram_addr.coarse_x = tram_addr.coarse_x;
        }
    };

    auto TransferAddressY = [&]() {
        if (mask.render_background || mask.render_sprites) {
            vram_addr.fine_y = tram_addr.fine_y;
            vram_addr.nametable_y = tram_addr.nametable_y;
            vram_addr.coarse_y = tram_addr.coarse_y;
        }
    };

    auto LoadBackgroundShifters = [&]() {
        bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
        bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;
        bg_shifter_attrib_lo = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
        bg_shifter_attrib_hi = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
    };

    auto UpdateShifters = [&]() {
        if (mask.render_background) {
            bg_shifter_pattern_lo <<= 1;
            bg_shifter_pattern_hi <<= 1;
            bg_shifter_attrib_lo <<= 1;
            bg_shifter_attrib_hi <<= 1;
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

    // All but 1 of the scanlines is visible to the user
    if (scanline >= -1 && scanline < 240) {
        if (scanline == 0 && cycle == 0) {
            // "Odd Frame" cycle skip
            cycle = 1;
        }

        if (scanline == -1 && cycle == 1) {
            // Start of new frame
            status.vertical_blank = 0;
            status.sprite_overflow = 0;
            status.sprite_zero_hit = 0;
        }

        if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338)) {
            UpdateShifters();

            switch ((cycle - 1) % 8) {
                case 0:
                    LoadBackgroundShifters();
                    bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
                    break;
                case 2:
                    bg_next_tile_attrib = ppuRead(0x23C0 | (vram_addr.nametable_y << 11) 
                                                     | (vram_addr.nametable_x << 10) 
                                                     | ((vram_addr.coarse_y >> 2) << 3) 
                                                     | (vram_addr.coarse_x >> 2));
                    if (vram_addr.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
                    if (vram_addr.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
                    bg_next_tile_attrib &= 0x03;
                    break;
                case 4:
                    bg_next_tile_lsb = ppuRead((control.pattern_background << 12) 
                                             + ((uint16_t)bg_next_tile_id << 4) 
                                             + (vram_addr.fine_y) + 0);
                    break;
                case 6:
                    bg_next_tile_msb = ppuRead((control.pattern_background << 12)
                                             + ((uint16_t)bg_next_tile_id << 4)
                                             + (vram_addr.fine_y) + 8);
                    break;
                case 7:
                    IncrementScrollX();
                    break;
            }
        }

        if (cycle == 256) {
            IncrementScrollY();
        }

        if (cycle == 257) {
            LoadBackgroundShifters();
            TransferAddressX();
        }

        if (cycle == 338 || cycle == 340) {
            bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
        }

        if (scanline == -1 && cycle >= 280 && cycle < 305) {
            TransferAddressY();
        }

        // Sprite evaluation
        if (cycle == 257 && scanline >= 0) {
            // Clear sprite scanline
            for (int i = 0; i < 8; i++) {
                spriteScanline[i] = {0xFF, 0xFF, 0xFF, 0xFF};
            }
            sprite_count = 0;
            
            // Clear sprite shifters
            for (int i = 0; i < 8; i++) {
                sprite_shifter_pattern_lo[i] = 0;
                sprite_shifter_pattern_hi[i] = 0;
            }

            uint8_t nOAMEntry = 0;
            bSpriteZeroHitPossible = false;

            while (nOAMEntry < 64 && sprite_count < 9) {
                int16_t diff = ((int16_t)scanline - (int16_t)OAM[nOAMEntry].y);
                if (diff >= 0 && diff < (control.sprite_size ? 16 : 8)) {
                    if (sprite_count < 8) {
                        if (nOAMEntry == 0) {
                            bSpriteZeroHitPossible = true;
                        }
                        spriteScanline[sprite_count] = OAM[nOAMEntry];
                        sprite_count++;
                    }
                }
                nOAMEntry++;
            }
            status.sprite_overflow = (sprite_count > 8);
        }

        if (cycle == 340) {
            for (uint8_t i = 0; i < sprite_count; i++) {
                uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
                uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;

                if (!control.sprite_size) {
                    // 8x8 Sprite Mode
                    if (!(spriteScanline[i].attribute & 0x80)) {
                        sprite_pattern_addr_lo = (control.pattern_sprite << 12)
                                               | (spriteScanline[i].id << 4)
                                               | (scanline - spriteScanline[i].y);
                    } else {
                        sprite_pattern_addr_lo = (control.pattern_sprite << 12)
                                               | (spriteScanline[i].id << 4)
                                               | (7 - (scanline - spriteScanline[i].y));
                    }
                } else {
                    // 8x16 Sprite Mode
                    if (!(spriteScanline[i].attribute & 0x80)) {
                        if (scanline - spriteScanline[i].y < 8) {
                            sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12)
                                                   | ((spriteScanline[i].id & 0xFE) << 4)
                                                   | ((scanline - spriteScanline[i].y) & 0x07);
                        } else {
                            sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12)
                                                   | (((spriteScanline[i].id & 0xFE) + 1) << 4)
                                                   | ((scanline - spriteScanline[i].y) & 0x07);
                        }
                    } else {
                        if (scanline - spriteScanline[i].y < 8) {
                            sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12)
                                                   | (((spriteScanline[i].id & 0xFE) + 1) << 4)
                                                   | (7 - (scanline - spriteScanline[i].y) & 0x07);
                        } else {
                            sprite_pattern_addr_lo = ((spriteScanline[i].id & 0x01) << 12)
                                                   | ((spriteScanline[i].id & 0xFE) << 4)
                                                   | (7 - (scanline - spriteScanline[i].y) & 0x07);
                        }
                    }
                }

                sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;
                sprite_pattern_bits_lo = ppuRead(sprite_pattern_addr_lo);
                sprite_pattern_bits_hi = ppuRead(sprite_pattern_addr_hi);

                // Handle horizontal flip
                if (spriteScanline[i].attribute & 0x40) {
                    auto flipbyte = [](uint8_t b) {
                        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                        return b;
                    };
                    sprite_pattern_bits_lo = flipbyte(sprite_pattern_bits_lo);
                    sprite_pattern_bits_hi = flipbyte(sprite_pattern_bits_hi);
                }

                sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
                sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
            }
        }
    }

    if (scanline == 240) {
        // Post Render Scanline - Do Nothing
    }

    if (scanline >= 241 && scanline < 261) {
        if (scanline == 241 && cycle == 1) {
            status.vertical_blank = 1;
            if (control.enable_nmi) 
                nmi = true;
        }
    }

    // Composition - Render pixel
    if (scanline >= 0 && scanline < 240 && cycle >= 1 && cycle <= 256) {
        uint8_t bg_pixel = 0x00;
        uint8_t bg_palette = 0x00;

        if (mask.render_background) {
            uint16_t bit_mux = 0x8000 >> fine_x;
            uint8_t p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
            uint8_t p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;
            bg_pixel = (p1_pixel << 1) | p0_pixel;

            uint8_t bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
            uint8_t bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
            bg_palette = (bg_pal1 << 1) | bg_pal0;
        }

        uint8_t fg_pixel = 0x00;
        uint8_t fg_palette = 0x00;
        uint8_t fg_priority = 0x00;

        if (mask.render_sprites) {
            bSpriteZeroBeingRendered = false;
            for (uint8_t i = 0; i < sprite_count; i++) {
                if (spriteScanline[i].x == 0) {
                    uint8_t fg_pixel_lo = (sprite_shifter_pattern_lo[i] & 0x80) > 0;
                    uint8_t fg_pixel_hi = (sprite_shifter_pattern_hi[i] & 0x80) > 0;
                    fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;
                    fg_palette = (spriteScanline[i].attribute & 0x03) + 0x04;
                    fg_priority = (spriteScanline[i].attribute & 0x20) == 0;

                    if (fg_pixel != 0) {
                        if (i == 0) {
                            bSpriteZeroBeingRendered = true;
                        }
                        break;
                    }
                }
            }
        }

        uint8_t pixel = 0x00;
        uint8_t palette = 0x00;

        if (bg_pixel == 0 && fg_pixel == 0) {
            pixel = 0x00;
            palette = 0x00;
        } else if (bg_pixel == 0 && fg_pixel > 0) {
            pixel = fg_pixel;
            palette = fg_palette;
        } else if (bg_pixel > 0 && fg_pixel == 0) {
            pixel = bg_pixel;
            palette = bg_palette;
        } else if (bg_pixel > 0 && fg_pixel > 0) {
            if (fg_priority) {
                pixel = fg_pixel;
                palette = fg_palette;
            } else {
                pixel = bg_pixel;
                palette = bg_palette;
            }

            // Sprite Zero Hit detection
            if (bSpriteZeroHitPossible && bSpriteZeroBeingRendered) {
                if (mask.render_background & mask.render_sprites) {
                    if (~(mask.render_background_left | mask.render_sprites_left)) {
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
        }

        // Draw the pixel
        framebuffer[scanline * 256 + (cycle - 1)] = GetColourFromPaletteRam(palette, pixel);
    }

    // Advance renderer
    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 261) {
            scanline = -1;
            frame_complete = true;
        }
    }
    static int debug_frame = 0;
    if (debug_frame % 60 == 0) {
        printf("PPU Frame %d: Control=%02X, Mask=%02X, Status=%02X, VRAM=%04X\n",
               debug_frame, control.reg, mask.reg, status.reg, vram_addr.reg);
        printf("  Rendering: BG=%d, SPR=%d, NMI=%d\n",
               mask.render_background, mask.render_sprites, control.enable_nmi);
    }
    debug_frame++;
}