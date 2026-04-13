#include "gui.h"
#include <filesystem>
#include <algorithm>
#include <cstdio>

namespace fs = std::filesystem;

static constexpr SDL_Color C_BG      = {0x12, 0x12, 0x1E, 0xFF};
static constexpr SDL_Color C_PANEL   = {0x1C, 0x1C, 0x2E, 0xFF};
static constexpr SDL_Color C_ACCENT  = {0x7C, 0x3A, 0xFF, 0xFF};
static constexpr SDL_Color C_ACCENT2 = {0xFF, 0x5C, 0x8A, 0xFF};
static constexpr SDL_Color C_SEL     = {0x2E, 0x2E, 0x48, 0xFF};
static constexpr SDL_Color C_SEL_HL  = {0x3E, 0x1E, 0x7E, 0xFF};
static constexpr SDL_Color C_TEXT    = {0xE8, 0xE8, 0xFF, 0xFF};
static constexpr SDL_Color C_SUBTEXT = {0x88, 0x88, 0xAA, 0xFF};
static constexpr SDL_Color C_BORDER  = {0x3A, 0x3A, 0x5A, 0xFF};
static constexpr SDL_Color C_GREEN   = {0x44, 0xDD, 0x88, 0xFF};

static int W = 1280;
static int H = 800;

static const char* BTN_NAMES[BTN_COUNT] = {
    "A", "B", "Select", "Start", "Up", "Down", "Left", "Right"
};


static bool inRect(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx < x + w && my >= y && my < y + h;
}

void GUI::applyFullscreen(SDL_Window* win, bool fullscreen) {
    if (fullscreen) {
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(win, 0);
        SDL_SetWindowSize(win, 1280, 800);
        SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

void GUI::drawRect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect rc{x, y, w, h};
    SDL_RenderFillRect(r, &rc);
}
void GUI::drawRectLine(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect rc{x, y, w, h};
    SDL_RenderDrawRect(r, &rc);
}
void GUI::drawText(SDL_Renderer* r, TTF_Font* f, const std::string& txt,
                   int x, int y, SDL_Color c) {
    if (txt.empty()) return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt.c_str(), c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst{x, y, s->w, s->h};
    SDL_RenderCopy(r, t, nullptr, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}
int GUI::textWidth(TTF_Font* f, const std::string& txt) {
    int w = 0;
    TTF_SizeUTF8(f, txt.c_str(), &w, nullptr);
    return w;
}

std::vector<std::string> GUI::scanRoms(const std::string& dir) {
    std::vector<std::string> out;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(dir, ec))
        if (e.path().extension() == ".nes")
            out.push_back(e.path().string());
    std::sort(out.begin(), out.end());
    return out;
}

std::string GUI::openDirDialog() {
    FILE* p = popen("osascript -e 'POSIX path of (choose folder with prompt \"Select ROM Folder\")'", "r");
    if (!p) return "";
    char buf[1024] = {};
    if (fgets(buf, sizeof(buf), p)) {
        pclose(p);
        std::string s(buf);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        return s;
    }
    pclose(p);
    return "";
}

static void drawToggle(SDL_Renderer* r, int x, int y, bool on, int mx, int my) {
    const int TW = 54, TH = 28;
    bool hov = inRect(mx, my, x, y, TW, TH);
    SDL_Color track;
    if (on)
        track = hov ? SDL_Color{0x9C, 0x5A, 0xFF, 0xFF} : SDL_Color{0x7C, 0x3A, 0xFF, 0xFF};
    else
        track = hov ? SDL_Color{0x5A, 0x5A, 0x7A, 0xFF} : SDL_Color{0x3A, 0x3A, 0x5A, 0xFF};
    SDL_SetRenderDrawColor(r, track.r, track.g, track.b, track.a);
    SDL_Rect rc{x, y, TW, TH};
    SDL_RenderFillRect(r, &rc);
    // Knob
    int kx = on ? x + TW - 26 : x + 2;
    SDL_SetRenderDrawColor(r, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_Rect knob{kx, y + 2, 24, TH - 4};
    SDL_RenderFillRect(r, &knob);
}

GuiScreen GUI::runLauncher(SDL_Window* win, SDL_Renderer* r, TTF_Font* fl, TTF_Font* fsmall,
                            Config& cfg, std::string& outRom) {
    auto roms = scanRoms(cfg.romDirectory);
    int  sel  = 0, scroll = 0;
    const int HEADER_H = 108;
    const int FOOTER_H = 44;
    const int ROW_H    = 40;

    SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);

    SDL_Event ev;
    bool dirty = true;
    int mx = 0, my = 0;

    while (true) {
        if (SDL_WaitEventTimeout(&ev, 16)) {
            dirty = true;
            int visibleRows = (H - HEADER_H - FOOTER_H) / ROW_H;

            switch (ev.type) {
                case SDL_QUIT: return GuiScreen::None;

                case SDL_KEYDOWN: {
                    SDL_Keycode k = ev.key.keysym.sym;
                    switch (k) {
                        case SDLK_UP:
                            if (sel > 0) { sel--; if (sel < scroll) scroll = sel; }
                            break;
                        case SDLK_DOWN:
                            if (sel < (int)roms.size()-1) {
                                sel++;
                                if (sel >= scroll + visibleRows) scroll = sel - visibleRows + 1;
                            }
                            break;
                        case SDLK_PAGEUP:
                            sel = std::max(0, sel - visibleRows);
                            scroll = std::max(0, scroll - visibleRows);
                            break;
                        case SDLK_PAGEDOWN:
                            sel = std::min((int)roms.size()-1, sel + visibleRows);
                            if (sel >= scroll + visibleRows) scroll = sel - visibleRows + 1;
                            break;
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            if (!roms.empty()) {
                                outRom = roms[sel];
                                auto& rec = cfg.recentRoms;
                                rec.erase(std::remove(rec.begin(), rec.end(), outRom), rec.end());
                                rec.insert(rec.begin(), outRom);
                                if (rec.size() > 10) rec.resize(10);
                                cfg.save();
                                return GuiScreen::None;
                            }
                            break;
                        case SDLK_TAB:
                        case SDLK_ESCAPE:
                            return GuiScreen::Settings;
                        case SDLK_o: {
                            std::string d = openDirDialog();
                            if (!d.empty()) {
                                cfg.romDirectory = d; cfg.save();
                                roms = scanRoms(cfg.romDirectory);
                                sel = scroll = 0;
                            }
                            break;
                        }
                        case SDLK_F11:
                            cfg.fullscreen = !cfg.fullscreen;
                            cfg.save();
                            applyFullscreen(win, cfg.fullscreen);
                            SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);
                            break;
                    }
                    break;
                }

                case SDL_MOUSEMOTION:
                    mx = ev.motion.x; my = ev.motion.y;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.button == SDL_BUTTON_LEFT) {
                        mx = ev.button.x; my = ev.button.y;
                        int bx = W - 370;
                        // Open Folder button
                        if (inRect(mx, my, bx, 28, 120, 38)) {
                            std::string d = openDirDialog();
                            if (!d.empty()) {
                                cfg.romDirectory = d; cfg.save();
                                roms = scanRoms(cfg.romDirectory); sel = scroll = 0;
                            }
                        }
                        // Settings button
                        if (inRect(mx, my, bx + 135, 28, 120, 38))
                            return GuiScreen::Settings;
                        // Exit button
                        if (inRect(mx, my, bx + 270, 28, 80, 38))
                            return GuiScreen::None;
                        // ROM list rows
                        if (my >= HEADER_H && my < H - FOOTER_H) {
                            int clicked = scroll + (my - HEADER_H) / ROW_H;
                            if (clicked >= 0 && clicked < (int)roms.size()) {
                                if (clicked == sel) { // second click = launch
                                    outRom = roms[sel];
                                    auto& rec = cfg.recentRoms;
                                    rec.erase(std::remove(rec.begin(), rec.end(), outRom), rec.end());
                                    rec.insert(rec.begin(), outRom);
                                    if (rec.size() > 10) rec.resize(10);
                                    cfg.save();
                                    return GuiScreen::None;
                                }
                                sel = clicked;
                            }
                        }
                    }
                    break;

                case SDL_MOUSEWHEEL:
                    {
                        int visR = (H - HEADER_H - FOOTER_H) / ROW_H;
                        scroll -= ev.wheel.y;
                        scroll = std::max(0, std::min(scroll, (int)roms.size() - visR));
                    }
                    SDL_GetMouseState(&mx, &my);
                    break;

                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_RESIZED ||
                        ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);
                    }
                    break;
            }
        }

        if (!dirty) continue;
        dirty = false;

        int visibleRows = (H - HEADER_H - FOOTER_H) / ROW_H;

        drawRect(r, 0, 0, W, H, C_BG);

        drawRect(r, 0, 0, W, HEADER_H, C_PANEL);
        drawRect(r, 0, HEADER_H - 4, W, 2, C_ACCENT);
        drawRect(r, 0, HEADER_H - 2, W, 2, C_ACCENT2);
        drawText(r, fl, "NESPC", 28, 18, C_ACCENT);
        drawText(r, fsmall, "NES Emulator", 28, 68, C_SUBTEXT);

        int bx = W - 370;
        SDL_Color openCol = inRect(mx, my, bx, 28, 120, 38)
                            ? SDL_Color{0x9C, 0x5A, 0xFF, 0xFF} : C_ACCENT;
        drawRect(r, bx, 28, 120, 38, openCol);
        drawText(r, fsmall, "  Open Folder", bx + 6, 39, C_TEXT);
        SDL_Color settCol = inRect(mx, my, bx + 135, 28, 120, 38) ? C_SEL_HL : C_SEL;
        drawRect(r, bx + 135, 28, 120, 38, settCol);
        drawRectLine(r, bx + 135, 28, 120, 38, C_ACCENT);
        drawText(r, fsmall, "  Settings", bx + 141, 39, C_TEXT);

        SDL_Color exitCol = inRect(mx, my, bx + 270, 28, 80, 38) ? SDL_Color{0xFF,0x66,0x66,0xFF} : SDL_Color{0xAA,0x22,0x22,0xFF};
        drawRect(r, bx + 270, 28, 80, 38, exitCol);
        drawText(r, fsmall, "  Exit", bx + 276, 39, C_TEXT);
        drawText(r, fsmall, "  " + cfg.romDirectory, 28, 111, C_SUBTEXT);

        for (int i = 0; i < visibleRows; i++) {
            int idx = scroll + i;
            if (idx >= (int)roms.size()) break;
            int ry = HEADER_H + i * ROW_H;
            bool isSel = (idx == sel);
            bool isHov = inRect(mx, my, 0, ry, W, ROW_H - 1) && !isSel;

            if (isSel) {
                drawRect(r, 0, ry, W, ROW_H - 1, C_SEL_HL);
                drawRect(r, 0, ry, 5, ROW_H - 1, C_ACCENT);
            } else if (isHov) {
                drawRect(r, 0, ry, W, ROW_H - 1, SDL_Color{0x2A, 0x2A, 0x40, 0xFF});
            } else if (idx % 2 == 0) {
                drawRect(r, 0, ry, W, ROW_H - 1, C_SEL);
            }

            std::string name = fs::path(roms[idx]).stem().string();
            while (textWidth(fsmall, name) > W - 160) name.pop_back();

            bool isRecent = std::find(cfg.recentRoms.begin(), cfg.recentRoms.end(),
                                      roms[idx]) != cfg.recentRoms.end();
            if (isRecent) {
                //drawRect(r, 14, ry + 9, 52, 20, C_ACCENT2);
                //drawText(r, fsmall, "RECENT", 16, ry + 11, C_BG);
            }
            drawText(r, fsmall, name, 22, ry + 11,
                     (isSel || isHov) ? C_TEXT : C_SUBTEXT);

            if (isSel) {
                std::string hint = "\xe2\x86\xb5 Launch";
                drawText(r, fsmall, hint, W - textWidth(fsmall, hint) - 18, ry + 11, C_ACCENT);
            } else if (isHov) {
                std::string hint = "Click again to launch";
                drawText(r, fsmall, hint, W - textWidth(fsmall, hint) - 18, ry + 11, C_SUBTEXT);
            }
        }

        if (roms.empty()) {
            drawText(r, fl, "No .nes files found.", W/2 - 150, H/2 - 24, C_SUBTEXT);
            drawText(r, fsmall, "Click 'Open Folder' or press O to choose a directory.", W/2 - 210, H/2 + 20, C_SUBTEXT);
        }

        drawRect(r, 0, H - FOOTER_H, W, FOOTER_H, C_PANEL);
        drawRect(r, 0, H - FOOTER_H, W, 1, C_BORDER);
        std::string ftrLeft = "  \xe2\x86\x91/\xe2\x86\x93 Navigate    \xe2\x86\xb5/Double-click Launch    O Open Folder    ESC Settings    F11 Fullscreen";
        std::string ftrRight = std::to_string(roms.size()) + " ROMs  ";
        drawText(r, fsmall, ftrLeft, 16, H - FOOTER_H + 14, C_SUBTEXT);
        drawText(r, fsmall, ftrRight, W - textWidth(fsmall, ftrRight) - 4, H - FOOTER_H + 14, C_SUBTEXT);

        SDL_RenderPresent(r);
    }
}

GuiScreen GUI::runSettings(SDL_Window* win, SDL_Renderer* r, TTF_Font* fl, TTF_Font* fsmall, Config& cfg) {
    SettingsTab tab       = SettingsTab::Controls;
    int bindingPlayer = 0, bindingBtn = -1, playerTab = 0;

    SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);

    SDL_Event ev;
    bool dirty = true;
    int mx = 0, my = 0;

    while (true) {
        if (SDL_WaitEventTimeout(&ev, 16)) {
            dirty = true;

            if (bindingBtn >= 0 && ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k != SDLK_ESCAPE)
                    cfg.players[bindingPlayer].keys[bindingBtn] = k;
                bindingBtn = -1;
                cfg.save();
                continue;
            }

            switch (ev.type) {
                case SDL_QUIT: return GuiScreen::None;

                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {
                        case SDLK_ESCAPE: return GuiScreen::Launcher;
                        case SDLK_1: tab = SettingsTab::Controls; break;
                        case SDLK_2: tab = SettingsTab::Emulator; break;
                        case SDLK_F11:
                            cfg.fullscreen = !cfg.fullscreen;
                            cfg.save();
                            applyFullscreen(win, cfg.fullscreen);
                            SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);
                            break;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    mx = ev.motion.x; my = ev.motion.y;
                    if (ev.motion.state & SDL_BUTTON_LMASK && tab == SettingsTab::Emulator) {
                        int sliderX = 24, sliderW = std::min(500, W - 100);
                        if (inRect(mx, ev.motion.y, sliderX - 10, 385, sliderW + 20, 38)) {
                            cfg.audioVolume = std::max(0.0f, std::min(1.0f,
                                (ev.motion.x - sliderX) / (float)sliderW));
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.button == SDL_BUTTON_LEFT) {
                        mx = ev.button.x; my = ev.button.y;

                        if (inRect(mx, my, W - 140, 24, 120, 38))
                            return GuiScreen::Launcher;

                        if (inRect(mx, my, 24, 112, 126, 36))  tab = SettingsTab::Controls;
                        if (inRect(mx, my, 164, 112, 126, 36)) tab = SettingsTab::Emulator;

                        if (tab == SettingsTab::Controls) {
                            if (inRect(mx, my, 24, 168, 64, 36))  playerTab = 0;
                            if (inRect(mx, my, 100, 168, 64, 36)) playerTab = 1;
                            if (inRect(mx, my, W - 180, 168, 156, 36)) {
                                cfg.players[0].keys = {SDLK_z, SDLK_x, SDLK_RSHIFT, SDLK_RETURN,
                                                       SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
                                cfg.players[1].keys = {SDLK_k, SDLK_l, SDLK_RSHIFT, SDLK_RETURN,
                                                       SDLK_w, SDLK_s, SDLK_a, SDLK_d};
                                cfg.save();
                            }

                            for (int b = 0; b < BTN_COUNT; b++) {
                                int ry = 218 + b * 48;
                                if (inRect(mx, my, W - 200, ry + 6, 180, 28)) {
                                    bindingBtn = b; bindingPlayer = playerTab;
                                }
                            }
                        }

                        if (tab == SettingsTab::Emulator) {
                            for (int s = 1; s <= 3; s++) {
                                int bx2 = 24 + (s-1) * 96;
                                if (inRect(mx, my, bx2, 266, 80, 36)) {
                                    cfg.windowScale = s; cfg.save();
                                }
                            }
                            {
                                int sliderX = 24, sliderW = std::min(500, W - 100);
                                if (inRect(mx, my, sliderX, 385, sliderW, 38)) {
                                    cfg.audioVolume = std::max(0.0f, std::min(1.0f,
                                        (mx - sliderX) / (float)sliderW));
                                    cfg.save();
                                }
                            }
                            if (inRect(mx, my, 24, 490, 54, 28)) {
                                cfg.fullscreen = !cfg.fullscreen;
                                cfg.save();
                                applyFullscreen(win, cfg.fullscreen);
                                SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);
                            }
                            if (inRect(mx, my, 24, 548, 176, 40)) {
                                std::string d = openDirDialog();
                                if (!d.empty()) { cfg.romDirectory = d; cfg.save(); }
                            }
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (tab == SettingsTab::Emulator) cfg.save();
                    break;

                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_RESIZED ||
                        ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);
                    }
                    break;
            }
        }

        if (!dirty) continue;
        dirty = false;

        drawRect(r, 0, 0, W, H, C_BG);

        drawRect(r, 0, 0, W, 100, C_PANEL);
        drawRect(r, 0, 97, W, 2, C_ACCENT2);
        drawRect(r, 0, 99, W, 2, C_ACCENT);
        drawText(r, fl, "Settings", 28, 18, C_ACCENT2);
        drawText(r, fsmall, "NESPC Emulator", 28, 66, C_SUBTEXT);

        SDL_Color backCol = inRect(mx, my, W - 140, 24, 120, 38) ? C_SEL_HL : C_SEL;
        drawRect(r, W - 140, 24, 120, 38, backCol);
        drawRectLine(r, W - 140, 24, 120, 38, C_ACCENT);
        drawText(r, fsmall, "\xe2\x86\x90 Back  [ESC]", W - 134, 34, C_TEXT);

        auto drawTab = [&](const char* label, int tx, bool active) {
            bool hov = inRect(mx, my, tx, 112, 126, 36) && !active;
            SDL_Color bg = active ? C_ACCENT : (hov ? C_SEL_HL : C_SEL);
            drawRect(r, tx, 112, 126, 36, bg);
            drawText(r, fsmall, label, tx + 10, 122, active ? C_TEXT : C_SUBTEXT);
        };
        drawTab("[1] Controls", 24,  tab == SettingsTab::Controls);
        drawTab("[2] Emulator", 164, tab == SettingsTab::Emulator);

        if (tab == SettingsTab::Controls) {
            auto drawPTab = [&](const char* label, int tx, bool active) {
                bool hov = inRect(mx, my, tx, 168, 64, 36) && !active;
                drawRect(r, tx, 168, 64, 36, active ? C_ACCENT : (hov ? C_SEL_HL : C_SEL));
                drawText(r, fsmall, label, tx + 18, 178, active ? C_TEXT : C_SUBTEXT);
            };
            drawPTab("P1", 24,  playerTab == 0);
            drawPTab("P2", 100, playerTab == 1);
            drawText(r, fsmall, "Click a key badge to remap, then press any key.", 196, 181, C_SUBTEXT);
            bool rstHov = inRect(mx, my, W - 180, 168, 156, 36);
            drawRect(r, W - 180, 168, 156, 36, rstHov ? SDL_Color{0xAA,0x44,0x22,0xFF} : SDL_Color{0x66,0x22,0x11,0xFF});
            drawRectLine(r, W - 180, 168, 156, 36, SDL_Color{0xFF,0x66,0x44,0xFF});
            drawText(r, fsmall, "  Reset Defaults", W - 174, 178, C_TEXT);

            for (int b = 0; b < BTN_COUNT; b++) {
                int ry = 218 + b * 48;
                bool isBinding = (bindingBtn == b && bindingPlayer == playerTab);
                bool rowHov    = inRect(mx, my, W - 200, ry + 6, 180, 28) && !isBinding;
                drawRect(r, 24, ry, W - 48, 40,
                         isBinding ? C_SEL_HL : (b % 2 == 0 ? C_SEL : C_PANEL));
                if (isBinding) drawRectLine(r, 24, ry, W - 48, 40, C_ACCENT2);
                drawText(r, fsmall, BTN_NAMES[b], 40, ry + 12, C_TEXT);

                SDL_Keycode kc = cfg.players[playerTab].keys[b];
                std::string kName = isBinding ? "< press any key >"
                                              : std::string(SDL_GetKeyName(kc));
                SDL_Color bagCol = isBinding   ? C_ACCENT2
                                 : rowHov      ? SDL_Color{0x9C, 0x5A, 0xFF, 0xFF}
                                               : C_ACCENT;
                int kw = textWidth(fsmall, kName) + 24;
                drawRect(r, W - kw - 30, ry + 6, kw, 28, bagCol);
                drawText(r, fsmall, kName, W - kw - 18, ry + 10,
                         isBinding ? C_BG : C_TEXT);
            }
        }

        if (tab == SettingsTab::Emulator) {
            drawText(r, fl, "Window Scale", 24, 200, C_TEXT);
            drawText(r, fsmall, "Emulator window size when a game is running.", 24, 236, C_SUBTEXT);
            for (int s = 1; s <= 3; s++) {
                int bx2 = 24 + (s-1) * 96;
                bool active = (cfg.windowScale == s);
                bool hov = inRect(mx, my, bx2, 266, 80, 36) && !active;
                drawRect(r, bx2, 266, 80, 36, active ? C_ACCENT : (hov ? C_SEL_HL : C_SEL));
                drawRectLine(r, bx2, 266, 80, 36, active ? C_ACCENT : C_BORDER);
                drawText(r, fsmall, std::to_string(s) + "x", bx2 + 26, 276,
                         active ? C_TEXT : C_SUBTEXT);
            }

            drawText(r, fl, "Audio Volume", 24, 340, C_TEXT);
            //drawText(r, fsmall, "Click or drag the slider.", 24, 374, C_SUBTEXT);
            int sliderX = 24, sliderW = std::min(500, W - 100);
            drawRect(r, sliderX, 390, sliderW, 24, C_SEL);
            drawRect(r, sliderX, 390, (int)(cfg.audioVolume * sliderW), 24, C_ACCENT);
            drawRectLine(r, sliderX, 390, sliderW, 24, C_BORDER);
            int knobX = sliderX + (int)(cfg.audioVolume * sliderW) - 4;
            drawRect(r, knobX, 386, 8, 32, C_ACCENT2);
            char volBuf[16]; snprintf(volBuf, sizeof(volBuf), "%d%%", (int)(cfg.audioVolume*100));
            drawText(r, fsmall, volBuf, sliderX + sliderW + 14, 392, C_TEXT);

            drawText(r, fl, "Fullscreen Mode", 24, 448, C_TEXT);
            //drawText(r, fsmall, "Applies to both the launcher and the game.  F11 toggles anytime.", 24, 476, C_SUBTEXT);
            drawToggle(r, 24, 490, cfg.fullscreen, mx, my);
            drawText(r, fsmall, cfg.fullscreen ? "Enabled" : "Disabled", 88, 494,
                     cfg.fullscreen ? C_GREEN : C_SUBTEXT);

            //drawText(r, fl, "ROM Folder", 24, 546, C_TEXT);
            //drawText(r, fsmall, cfg.romDirectory, 24, 576, C_SUBTEXT);
            bool browsHov = inRect(mx, my, 24, 548, 176, 40);
            drawRect(r, 24, 548, 176, 40, browsHov ? SDL_Color{0x9C, 0x5A, 0xFF, 0xFF} : C_ACCENT);
            drawText(r, fsmall, "  Browse...  [O]", 28, 559, C_TEXT);
        }

        drawRect(r, 0, H - 44, W, 44, C_PANEL);
        drawRect(r, 0, H - 44, W, 1, C_BORDER);
        drawText(r, fsmall, "  ESC: Back to Launcher    1/2: Switch Tabs    F11: Toggle Fullscreen", 16, H - 30, C_SUBTEXT);

        SDL_RenderPresent(r);
    }
}

std::string GUI::run(SDL_Window* win, SDL_Renderer* r, Config& cfg) {
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return "";
    }

    const char* fontPaths[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/Library/Fonts/Arial.ttf",
        nullptr
    };
    TTF_Font* fontLarge = nullptr;
    TTF_Font* fontSmall  = nullptr;
    for (int i = 0; fontPaths[i] && !fontLarge; i++) {
        fontLarge = TTF_OpenFont(fontPaths[i], 28);
        fontSmall  = TTF_OpenFont(fontPaths[i], 17);
    }
    if (!fontLarge || !fontSmall) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
        TTF_Quit(); return "";
    }

    SDL_SetWindowTitle(win, "NESPC \xe2\x80\x93 Launcher");
    applyFullscreen(win, cfg.fullscreen);
    SDL_GetWindowSize(SDL_RenderGetWindow(r), &W, &H);

    std::string romPath;
    GuiScreen cur = GuiScreen::Launcher;

    while (cur != GuiScreen::None) {
        if (cur == GuiScreen::Launcher)
            cur = runLauncher(win, r, fontLarge, fontSmall, cfg, romPath);
        else if (cur == GuiScreen::Settings)
            cur = runSettings(win, r, fontLarge, fontSmall, cfg);
    }

    TTF_CloseFont(fontLarge);
    TTF_CloseFont(fontSmall);
    TTF_Quit();
    return romPath;
}
