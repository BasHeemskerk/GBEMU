#include <3ds.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>

#include "include/gui.hpp"
#include "gb/included/gb.hpp"
#include "gb/included/ppu.hpp"
#include "gb/included/joypad.hpp"
#include "gb/included/cartridge.hpp"
#include "gb/included/cpu.hpp"
#include "gb/included/memory.hpp"
#include "gb/included/apu.hpp"

int main(int argc, char** argv) {
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_BGR8_OES);

    romfsInit();
    gui::initialize();

    while (aptMainLoop()) {
        // single hidScanInput call per frame
        hidScanInput();
        u32 held = hidKeysHeld();

        if ((held & KEY_SELECT) && (held & KEY_START)) {
            break;
        }

        gui::update();

        if (gui::currentState == gui::State::RUNNING) {
            gb::joypad::update();
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
