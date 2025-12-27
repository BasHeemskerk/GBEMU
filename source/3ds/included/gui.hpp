// 3ds/gui.hpp
#ifndef GUI_3DS_HPP
#define GUI_3DS_HPP

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

    constexpr int VISIBLE_ITEMS = 8;

    void initialize();
    void update();
    void render();

}

#endif