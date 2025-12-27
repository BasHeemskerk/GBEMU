// 3ds/gui.cpp
#include "gui.hpp"
#include "../wrapper/included/gameboy.hpp"
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

extern GameBoy gb_obj;

namespace gui {

    State currentState;
    std::vector<std::string> romList;
    int selectedRom;
    
    static int scrollOffset;
    static int animFrame;
    static bool scannedOnce = false;

    // Colors
    static constexpr u32 COL_BG_DARK   = 0xFF0D0D1A;
    static constexpr u32 COL_BG_MID    = 0xFF1A1A2E;
    static constexpr u32 COL_BG_LIGHT  = 0xFF25253D;
    static constexpr u32 COL_ACCENT    = 0xFF00E676;
    static constexpr u32 COL_WHITE     = 0xFFFFFFFF;
    static constexpr u32 COL_GRAY      = 0xFF888888;
    static constexpr u32 COL_DARK_GRAY = 0xFF444444;
    static constexpr u32 COL_SELECTED  = 0xFF2D4A6E;

    // 8x8 font
    static const uint8_t font8x8[96][8] = {
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
        {0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00},{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
        {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},{0x00,0x66,0xAC,0xD8,0x36,0x6A,0xCC,0x00},
        {0x38,0x6C,0x68,0x76,0xDC,0xCE,0x7B,0x00},{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
        {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
        {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00},
        {0x7C,0xCE,0xDE,0xF6,0xE6,0xC6,0x7C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
        {0x7C,0xC6,0x0E,0x3C,0x78,0xE0,0xFE,0x00},{0x7E,0x0C,0x18,0x3C,0x06,0xC6,0x7C,0x00},
        {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00},{0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},
        {0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},{0xFE,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
        {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},{0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},
        {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
        {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
        {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},{0x3C,0x66,0x06,0x1C,0x18,0x00,0x18,0x00},
        {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x7E,0x00},{0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00},
        {0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00},{0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00},
        {0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00},{0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xFE,0x00},
        {0xFE,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x00},{0x7C,0xC6,0xC0,0xCE,0xC6,0xC6,0x7E,0x00},
        {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},{0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},
        {0x06,0x06,0x06,0x06,0xC6,0xC6,0x7C,0x00},{0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00},
        {0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00},{0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
        {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},{0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
        {0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00},{0x7C,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x06},
        {0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00},{0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00},
        {0xFF,0x18,0x18,0x18,0x18,0x18,0x18,0x00},{0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xFE,0x00},
        {0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00},{0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
        {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00},{0xC3,0xC3,0x66,0x3C,0x18,0x18,0x18,0x00},
        {0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE,0x00},{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
        {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
        {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
        {0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x7C,0x06,0x7E,0xC6,0x7E,0x00},
        {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xFC,0x00},{0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00},
        {0x06,0x06,0x7E,0xC6,0xC6,0xC6,0x7E,0x00},{0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00},
        {0x1C,0x36,0x30,0x78,0x30,0x30,0x30,0x00},{0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x7C},
        {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x00},{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
        {0x06,0x00,0x06,0x06,0x06,0x06,0xC6,0x7C},{0xC0,0xC0,0xCC,0xD8,0xF0,0xD8,0xCC,0x00},
        {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00},
        {0x00,0x00,0xFC,0xC6,0xC6,0xC6,0xC6,0x00},{0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00},
        {0x00,0x00,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0},{0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x06},
        {0x00,0x00,0xDC,0xE6,0xC0,0xC0,0xC0,0x00},{0x00,0x00,0x7E,0xC0,0x7C,0x06,0xFC,0x00},
        {0x30,0x30,0x7C,0x30,0x30,0x36,0x1C,0x00},{0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x00},
        {0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00},{0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},
        {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},{0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0x7C},
        {0x00,0x00,0xFE,0x0C,0x38,0x60,0xFE,0x00},{0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00},
        {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},{0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00},
        {0x72,0x9C,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    };

    // Drawing functions
    static void drawRect(u8* fb, int x, int y, int w, int h, u32 color, bool top) {
        u8 r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = color & 0xFF;
        int sH = 240, sW = top ? 400 : 320;
        for (int py = y; py < y + h && py < sH; py++) {
            for (int px = x; px < x + w && px < sW; px++) {
                if (px < 0 || py < 0) continue;
                int idx = ((px * sH) + (sH - 1 - py)) * 3;
                fb[idx] = b; fb[idx+1] = g; fb[idx+2] = r;
            }
        }
    }

    static void drawRectGradient(u8* fb, int x, int y, int w, int h, u32 cT, u32 cB, bool top) {
        int sH = 240, sW = top ? 400 : 320;
        for (int py = y; py < y + h && py < sH; py++) {
            float t = (float)(py - y) / (float)h;
            u8 r = ((cT >> 16) & 0xFF) * (1-t) + ((cB >> 16) & 0xFF) * t;
            u8 g = ((cT >> 8) & 0xFF) * (1-t) + ((cB >> 8) & 0xFF) * t;
            u8 b = (cT & 0xFF) * (1-t) + (cB & 0xFF) * t;
            for (int px = x; px < x + w && px < sW; px++) {
                if (px < 0 || py < 0) continue;
                int idx = ((px * sH) + (sH - 1 - py)) * 3;
                fb[idx] = b; fb[idx+1] = g; fb[idx+2] = r;
            }
        }
    }

    static void drawText(u8* fb, int x, int y, const char* text, u32 color, bool large, bool top) {
        u8 r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = color & 0xFF;
        int scale = large ? 2 : 1, cx = x, sH = 240, sW = top ? 400 : 320;
        while (*text) {
            char c = *text++;
            if (c < 32 || c > 127) c = '?';
            const uint8_t* glyph = font8x8[c - 32];
            for (int row = 0; row < 8; row++) {
                uint8_t rowData = glyph[row];
                for (int col = 0; col < 8; col++) {
                    if (rowData & (0x80 >> col)) {
                        for (int sy = 0; sy < scale; sy++) {
                            for (int sx = 0; sx < scale; sx++) {
                                int px = cx + col * scale + sx, py = y + row * scale + sy;
                                if (px >= 0 && px < sW && py >= 0 && py < sH) {
                                    int idx = ((px * sH) + (sH - 1 - py)) * 3;
                                    fb[idx] = b; fb[idx+1] = g; fb[idx+2] = r;
                                }
                            }
                        }
                    }
                }
            }
            cx += 8 * scale;
        }
    }

    static void drawTextCentered(u8* fb, int y, const char* text, u32 color, bool large, bool top) {
        int sW = top ? 400 : 320, cW = large ? 16 : 8, tW = strlen(text) * cW;
        drawText(fb, (sW - tW) / 2, y, text, color, large, top);
    }

    // ROM scanning
    static void scanForRoms(const char* directory) {
        DIR* dir = opendir(directory);
        if (!dir) return;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string filename = entry->d_name;
            size_t len = filename.length();
            if (len > 3) {
                std::string ext = filename.substr(len - 3);
                std::string ext4 = len > 4 ? filename.substr(len - 4) : "";
                for (char& c : ext) c = tolower(c);
                for (char& c : ext4) c = tolower(c);
                if (ext == ".gb" || ext4 == ".gbc") {
                    romList.push_back(std::string(directory) + "/" + filename);
                }
            }
        }
        closedir(dir);
    }

    static void doScan() {
        romList.clear();
        scanForRoms("romfs:/");
        scanForRoms("sdmc:/3ds/gb_roms");
        scanForRoms("sdmc:/gb_roms");
        scanForRoms("sdmc:/roms");
        std::sort(romList.begin(), romList.end());
        romList.erase(std::unique(romList.begin(), romList.end()), romList.end());
    }

    // Screen renderers
    static void drawMainMenu() {
        u8* ft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        
        drawRectGradient(ft, 0, 0, 400, 240, COL_BG_DARK, COL_BG_MID, true);
        int lineX = (animFrame * 2) % 500 - 50;
        drawRect(ft, lineX, 50, 100, 3, COL_ACCENT, true);
        drawTextCentered(ft, 72, "GAME BOY", 0xFF000000, true, true);
        drawTextCentered(ft, 70, "GAME BOY", COL_ACCENT, true, true);
        drawTextCentered(ft, 102, "EMULATOR", 0xFF000000, true, true);
        drawTextCentered(ft, 100, "EMULATOR", COL_WHITE, true, true);
        drawTextCentered(ft, 150, "for Nintendo 3DS", COL_GRAY, false, true);
        
        char dbg[64];
        snprintf(dbg, sizeof(dbg), "ROMs found: %d", (int)romList.size());
        drawTextCentered(ft, 180, dbg, COL_ACCENT, false, true);
        drawText(ft, 10, 220, "v1.0", COL_DARK_GRAY, false, true);
        
        drawRectGradient(fb, 0, 0, 320, 240, COL_BG_MID, COL_BG_DARK, false);
        drawRect(fb, 40, 60, 240, 80, COL_BG_LIGHT, false);
        drawRect(fb, 42, 62, 236, 76, COL_BG_DARK, false);
        
        u32 pulse = (animFrame / 30) % 2 ? COL_ACCENT : COL_WHITE;
        if (romList.empty()) {
            drawTextCentered(fb, 85, "No ROMs found!", 0xFFFF4444, false, false);
            drawTextCentered(fb, 105, "Press A to rescan", COL_GRAY, false, false);
        } else {
            drawTextCentered(fb, 90, "Press A to Start", pulse, false, false);
        }
        drawTextCentered(fb, 170, "ROMs: /3ds/gb_roms/", COL_GRAY, false, false);
        drawTextCentered(fb, 220, "SELECT+START to exit", COL_DARK_GRAY, false, false);
    }

    static void drawRomSelect() {
        u8* ft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        
        drawRectGradient(ft, 0, 0, 400, 240, COL_BG_DARK, COL_BG_MID, true);
        drawRect(ft, 0, 0, 400, 45, COL_BG_LIGHT, true);
        drawTextCentered(ft, 12, "ROM SELECTION", COL_ACCENT, true, true);
        
        if (!romList.empty() && selectedRom < (int)romList.size()) {
            std::string path = romList[selectedRom];
            size_t sl = path.find_last_of('/');
            std::string fn = (sl != std::string::npos) ? path.substr(sl + 1) : path;
            drawRect(ft, 30, 70, 340, 60, COL_BG_LIGHT, true);
            drawRect(ft, 32, 72, 336, 56, COL_BG_DARK, true);
            if (fn.length() > 30) fn = fn.substr(0, 27) + "...";
            drawTextCentered(ft, 92, fn.c_str(), COL_ACCENT, false, true);
            
            char cs[32];
            snprintf(cs, sizeof(cs), "%d / %d", selectedRom + 1, (int)romList.size());
            drawTextCentered(ft, 150, cs, COL_WHITE, false, true);
        }
        drawTextCentered(ft, 200, "A: Load   B: Back   D-Pad: Navigate", COL_GRAY, false, true);
        
        drawRectGradient(fb, 0, 0, 320, 240, COL_BG_MID, COL_BG_DARK, false);
        int sY = 10, iH = 26;
        for (int i = 0; i < VISIBLE_ITEMS && (scrollOffset + i) < (int)romList.size(); i++) {
            int ri = scrollOffset + i, y = sY + i * iH;
            if (ri == selectedRom) {
                drawRect(fb, 5, y, 290, iH - 2, COL_SELECTED, false);
                drawRect(fb, 5, y, 3, iH - 2, COL_ACCENT, false);
            }
            std::string path = romList[ri];
            size_t sl = path.find_last_of('/');
            std::string fn = (sl != std::string::npos) ? path.substr(sl + 1) : path;
            if (fn.length() > 32) fn = fn.substr(0, 29) + "...";
            drawText(fb, 15, y + 7, fn.c_str(), (ri == selectedRom) ? COL_WHITE : COL_GRAY, false, false);
        }
        
        if (romList.size() > VISIBLE_ITEMS) {
            int bH = 200, bY = 20;
            int tH = std::max(20, (int)((VISIBLE_ITEMS * bH) / romList.size()));
            int tY = bY + (scrollOffset * (bH - tH)) / std::max(1, (int)romList.size() - VISIBLE_ITEMS);
            drawRect(fb, 302, bY, 8, bH, COL_BG_LIGHT, false);
            drawRect(fb, 303, tY, 6, tH, COL_ACCENT, false);
        }
    }

    static void drawPauseMenu() {
        u8* ft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        
        // Dim the game screen
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 400; x++) {
                int idx = ((x * 240) + (239 - y)) * 3;
                ft[idx] /= 3;
                ft[idx+1] /= 3;
                ft[idx+2] /= 3;
            }
        }
        drawTextCentered(ft, 100, "PAUSED", COL_ACCENT, true, true);
        
        drawRectGradient(fb, 0, 0, 320, 240, COL_BG_MID, COL_BG_DARK, false);
        drawRect(fb, 50, 50, 220, 140, COL_BG_LIGHT, false);
        drawRect(fb, 52, 52, 216, 136, COL_BG_DARK, false);
        drawTextCentered(fb, 65, "GAME PAUSED", COL_ACCENT, true, false);
        drawTextCentered(fb, 110, "A: Resume Game", COL_WHITE, false, false);
        drawTextCentered(fb, 135, "B: Exit to Menu", COL_GRAY, false, false);
    }

    static void renderGameScreen() {
        u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        const uint8_t* gbFB = gb_obj.getFramebuffer();

        static const u8 gbColorsR[4] = {155, 139, 48, 15};
        static const u8 gbColorsG[4] = {188, 172, 98, 56};
        static const u8 gbColorsB[4] = {15, 15, 48, 15};

        const u8 borderR = 15, borderG = 40, borderB = 15;
        const int outW = 266;
        const int offsetX = (400 - outW) / 2;

        for (int screenY = 0; screenY < 240; screenY++) {
            int fbRowBase = 239 - screenY;
            int gbY = (screenY * 144) / 240;
            if (gbY >= 144) gbY = 143;

            for (int screenX = 0; screenX < 400; screenX++) {
                int idx = (screenX * 240 + fbRowBase) * 3;

                if (screenX < offsetX || screenX >= offsetX + outW) {
                    fb[idx] = borderB;
                    fb[idx+1] = borderG;
                    fb[idx+2] = borderR;
                } else {
                    int gbX = ((screenX - offsetX) * 160) / outW;
                    if (gbX >= 160) gbX = 159;

                    uint8_t ci = gbFB[gbY * 160 + gbX] & 0x03;
                    fb[idx] = gbColorsB[ci];
                    fb[idx+1] = gbColorsG[ci];
                    fb[idx+2] = gbColorsR[ci];
                }
            }
        }
    }

    // Public functions
    void initialize() {
        currentState = State::MAIN_MENU;
        selectedRom = 0;
        scrollOffset = 0;
        animFrame = 0;
        romList.clear();
        scannedOnce = false;
    }

    void update() {
        u32 kDown = hidKeysDown();
        animFrame++;
        
        if (!scannedOnce) {
            doScan();
            scannedOnce = true;
        }

        switch (currentState) {
            case State::MAIN_MENU:
                if (kDown & KEY_A) {
                    if (romList.empty()) doScan();
                    if (!romList.empty()) {
                        currentState = State::ROM_SELECT;
                        selectedRom = 0;
                        scrollOffset = 0;
                    }
                }
                break;

            case State::ROM_SELECT:
                if (kDown & KEY_UP && selectedRom > 0) {
                    selectedRom--;
                    if (selectedRom < scrollOffset) scrollOffset = selectedRom;
                }
                if (kDown & KEY_DOWN && selectedRom < (int)romList.size() - 1) {
                    selectedRom++;
                    if (selectedRom >= scrollOffset + VISIBLE_ITEMS) {
                        scrollOffset = selectedRom - VISIBLE_ITEMS + 1;
                    }
                }
                if (kDown & KEY_A) {
                    gb_obj.reset();
                    if (gb_obj.loadROM(romList[selectedRom].c_str())) {
                        currentState = State::RUNNING;
                    }
                }
                if (kDown & KEY_B) {
                    currentState = State::MAIN_MENU;
                }
                break;

            case State::RUNNING:
                if (kDown & KEY_SELECT) {
                    currentState = State::PAUSED;
                }
                break;

            case State::PAUSED:
                if (kDown & KEY_A) currentState = State::RUNNING;
                if (kDown & KEY_B) currentState = State::ROM_SELECT;
                break;
        }
    }

    void render() {
        switch (currentState) {
            case State::MAIN_MENU:  drawMainMenu(); break;
            case State::ROM_SELECT: drawRomSelect(); break;
            case State::RUNNING:    renderGameScreen(); break;
            case State::PAUSED:     renderGameScreen(); drawPauseMenu(); break;
        }
    }

}