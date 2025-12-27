#include <3ds.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>

#include "include/gui.hpp"
#include "gb/included/gb.hpp"
#include "gb/included/state.hpp"
#include "gb/included/joypad.hpp"
#include "gb/included/apu.hpp"

// Update joypad from 3DS input
void updateJoypad() {
    u32 held = hidKeysHeld();
    gb::GBState& state = gb::getState();
    
    gb::joypad::setButton(state, gb::joypad::BTN_A, held & KEY_A);
    gb::joypad::setButton(state, gb::joypad::BTN_B, held & KEY_B);
    gb::joypad::setButton(state, gb::joypad::BTN_START, held & KEY_START);
    gb::joypad::setButton(state, gb::joypad::BTN_SELECT, held & KEY_SELECT);
    gb::joypad::setButton(state, gb::joypad::BTN_UP, held & KEY_UP);
    gb::joypad::setButton(state, gb::joypad::BTN_DOWN, held & KEY_DOWN);
    gb::joypad::setButton(state, gb::joypad::BTN_LEFT, held & KEY_LEFT);
    gb::joypad::setButton(state, gb::joypad::BTN_RIGHT, held & KEY_RIGHT);
}

int main(int argc, char** argv) {
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_BGR8_OES);

    romfsInit();
    gui::initialize();
    gb::apu::initAudio();

    while (aptMainLoop()) {
        hidScanInput();
        u32 held = hidKeysHeld();

        if ((held & KEY_SELECT) && (held & KEY_START)) {
            break;
        }

        gui::update();

        if (gui::currentState == gui::State::RUNNING) {
            updateJoypad();
            gb::runFrame();
        }

        gui::render();

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gb::apu::exitAudio();
    romfsExit();
    gfxExit();
    return 0;
}
