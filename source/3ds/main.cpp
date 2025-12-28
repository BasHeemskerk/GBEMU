// 3ds/main.cpp
#include <3ds.h>
#include "gui.hpp"
#include "../wrapper/included/gameboy.hpp"

GameBoy gb_obj;

static void updateInput() {
    u32 held = hidKeysHeld();
    
    gb_obj.input.a      = held & KEY_A;
    gb_obj.input.b      = held & KEY_B;
    gb_obj.input.start  = held & KEY_START;
    gb_obj.input.select = held & KEY_SELECT;
    gb_obj.input.up     = held & KEY_UP;
    gb_obj.input.down   = held & KEY_DOWN;
    gb_obj.input.left   = held & KEY_LEFT;
    gb_obj.input.right  = held & KEY_RIGHT;
}

int main() {
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_BGR8_OES);
    
    romfsInit();
    
    gb_obj.init();

    //add our opcode table from romfs
    if (!gb_obj.loadOpcodeTable("romfs:/opcodes/default.gb_opcode")) {
        // Debug: show error
        consoleInit(GFX_BOTTOM, NULL);
        printf("Failed to load opcode table!\n");
        printf("Press START to exit\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 1;
    }

    gui::initialize();

    while (aptMainLoop()) {
        hidScanInput();
        u32 held = hidKeysHeld();
        
        if ((held & KEY_START) && (held & KEY_SELECT)) {
            break;
        }

        gui::update();

        if (gui::currentState == gui::State::RUNNING) {
            updateInput();
            gb_obj.runFrame();
        }

        gui::render();

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    romfsExit();
    gfxExit();
    return 0;
}