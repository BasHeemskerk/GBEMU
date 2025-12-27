#include "included/gameboy.hpp"
#include "../gb/included/gb.hpp"
#include "../gb/included/state.hpp"
#include "../gb/included/joypad.hpp"

GameBoy::GameBoy() : romLoaded(false) {
    input.clear();
}

GameBoy::~GameBoy() {
    // cleanup if needed
}

void GameBoy::init() {
    gb::initialize();
    romLoaded = false;
    input.clear();
}

void GameBoy::reset() {
    gb::initialize();
    input.clear();
}

void GameBoy::runFrame() {
    if (!romLoaded) {
        return;
    }

    updateInput();
    gb::runFrame();
}

void GameBoy::step() {
    if (!romLoaded) {
        return;
    }

    updateInput();
    gb::step();
}

bool GameBoy::loadROM(const char* filepath) {
    romLoaded = gb::loadROM(filepath);
    return romLoaded;
}

bool GameBoy::isROMLoaded() const {
    return romLoaded;
}

const char* GameBoy::getROMTitle() const {
    return gb::getROMTitle();
}

void GameBoy::updateInput() {
    gb::GBState& state = gb::getState();
    
    gb::joypad::setButton(state, gb::joypad::BTN_A, input.a);
    gb::joypad::setButton(state, gb::joypad::BTN_B, input.b);
    gb::joypad::setButton(state, gb::joypad::BTN_START, input.start);
    gb::joypad::setButton(state, gb::joypad::BTN_SELECT, input.select);
    gb::joypad::setButton(state, gb::joypad::BTN_UP, input.up);
    gb::joypad::setButton(state, gb::joypad::BTN_DOWN, input.down);
    gb::joypad::setButton(state, gb::joypad::BTN_LEFT, input.left);
    gb::joypad::setButton(state, gb::joypad::BTN_RIGHT, input.right);
}

uint8_t GameBoy::getInputState() const {
    uint8_t state = 0;
    if (input.a)      state |= 0x01;
    if (input.b)      state |= 0x02;
    if (input.select) state |= 0x04;
    if (input.start)  state |= 0x08;
    if (input.up)     state |= 0x10;
    if (input.down)   state |= 0x20;
    if (input.left)   state |= 0x40;
    if (input.right)  state |= 0x80;
    return state;
}

void GameBoy::setInputState(uint8_t state) {
    input.a      = state & 0x01;
    input.b      = state & 0x02;
    input.select = state & 0x04;
    input.start  = state & 0x08;
    input.up     = state & 0x10;
    input.down   = state & 0x20;
    input.left   = state & 0x40;
    input.right  = state & 0x80;
}

uint8_t* GameBoy::getFramebuffer() {
    return gb::getFramebuffer();
}

const uint8_t* GameBoy::getFramebuffer() const {
    return gb::getFramebuffer();
}

bool GameBoy::isFrameReady() const {
    return gb::isFrameReady();
}

void GameBoy::clearFrameReady() {
    gb::setFrameReady(false);
}

int16_t* GameBoy::getAudioBuffer() {
    return gb::getAudioBuffer();
}

int GameBoy::getAudioBufferPosition() const {
    return gb::getAudioBufferPosition();
}

void GameBoy::clearAudioBuffer() {
    gb::setAudioBufferPosition(0);
}

bool GameBoy::hasSRAM() const {
    return gb::getRAMSize() > 0;
}

bool GameBoy::saveSRAM(const char* filepath) {
    // TODO: implement
    return false;
}

bool GameBoy::loadSRAM(const char* filepath) {
    // TODO: implement
    return false;
}
