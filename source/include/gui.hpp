#ifndef GUI_HPP
#define GUI_HPP

#include <3ds.h>
#include <vector>
#include <string>

namespace gui {

    enum class State {
        MAIN_MENU,
        ROM_SELECT,
        RUNNING,
        PAUSED
    };

    extern State currentState;
    extern std::vector<std::string> romList;
    extern int selectedRom;
    extern int scrollOffset;
    extern int animFrame;

    constexpr int VISIBLE_ITEMS = 8;

    void initialize();
    void scanForRoms(const char* directory);
    void update();
    void render();
    void renderGameScreen();

    void drawRect(u8* fb, int x, int y, int w, int h, u32 color, bool topScreen = false);
    void drawRectGradient(u8* fb, int x, int y, int w, int h, u32 colorTop, u32 colorBottom, bool topScreen = false);
    void drawText(u8* fb, int x, int y, const char* text, u32 color, bool large = false, bool topScreen = false);
    void drawTextCentered(u8* fb, int y, const char* text, u32 color, bool large = false, bool topScreen = false);

    void drawMainMenu();
    void drawRomSelect();
    void drawPauseMenu();

    std::string getSelectedRomPath();

}

#endif