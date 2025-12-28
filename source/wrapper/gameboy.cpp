// gameboy.cpp
#include "included/gameboy.hpp"
#include "../gb/included/cpu.hpp"
#include "../gb/included/memory.hpp"
#include "../gb/included/ppu.hpp"
#include "../gb/included/apu.hpp"
#include "../gb/included/timer.hpp"
#include "../gb/included/joypad.hpp"
#include "../gb/included/cartridge.hpp"

GameBoy::GameBoy() : romLoaded(false) {
    input.clear();
}

GameBoy::~GameBoy() {
    gb::cartridge::cleanup(state);
}

void GameBoy::init() {
    gb::cpu::initialize(state);
    gb::memory::initialize(state);
    gb::cartridge::initialize(state);
    gb::ppu::initialize(state);
    gb::timer::initialize(state);
    gb::joypad::initialize(state);
    gb::apu::initialize(state);
    romLoaded = false;
    input.clear();
}

void GameBoy::reset() {
    init();
}

void GameBoy::runFrame() {
    if (!romLoaded) return;
    
    updateInput();
    state.ppu.frameReady = false;
    
    while (!state.ppu.frameReady) {
        step();
    }
}

void GameBoy::step() {
    int cycles = gb::cpu::step(state);
    gb::ppu::tick(state, cycles);
    gb::timer::tick(state, cycles);
    gb::apu::tick(state, cycles);
    handleInterrupts();
}

void GameBoy::handleInterrupts() {
    if (!state.cpu.ime) return;
    
    uint8_t pending = state.memory.io[gb::memory::IO_IF] & state.memory.ie & 0x1F;
    if (pending == 0) return;
    
    state.cpu.halted = false;
    
    uint16_t handler = 0;
    uint8_t bit = 0;
    
    if (pending & 0x01)      { handler = 0x0040; bit = 0x01; }
    else if (pending & 0x02) { handler = 0x0048; bit = 0x02; }
    else if (pending & 0x04) { handler = 0x0050; bit = 0x04; }
    else if (pending & 0x08) { handler = 0x0058; bit = 0x08; }
    else if (pending & 0x10) { handler = 0x0060; bit = 0x10; }
    
    if (handler) {
        state.cpu.ime = false;
        state.memory.io[gb::memory::IO_IF] &= ~bit;
        state.cpu.SP--;
        gb::memory::write(state, state.cpu.SP, state.cpu.PC >> 8);
        state.cpu.SP--;
        gb::memory::write(state, state.cpu.SP, state.cpu.PC & 0xFF);
        state.cpu.PC = handler;
    }
}

bool GameBoy::loadROM(const char* filepath) {
    romLoaded = gb::cartridge::loadRom(state, filepath);
    return romLoaded;
}

void GameBoy::updateInput() {
    gb::joypad::setButton(state, gb::joypad::BTN_A, input.a);
    gb::joypad::setButton(state, gb::joypad::BTN_B, input.b);
    gb::joypad::setButton(state, gb::joypad::BTN_START, input.start);
    gb::joypad::setButton(state, gb::joypad::BTN_SELECT, input.select);
    gb::joypad::setButton(state, gb::joypad::BTN_UP, input.up);
    gb::joypad::setButton(state, gb::joypad::BTN_DOWN, input.down);
    gb::joypad::setButton(state, gb::joypad::BTN_LEFT, input.left);
    gb::joypad::setButton(state, gb::joypad::BTN_RIGHT, input.right);
}

uint8_t* GameBoy::getFramebuffer() {
    return state.ppu.framebuffer;
}

const uint8_t* GameBoy::getFramebuffer() const {
    return state.ppu.framebuffer;
}

bool GameBoy::isFrameReady() const {
    return state.ppu.frameReady;
}

void GameBoy::clearFrameReady() {
    state.ppu.frameReady = false;
}

int16_t* GameBoy::getAudioBuffer() {
    return state.apu.audioBuffer;
}

int GameBoy::getAudioBufferPosition() const {
    return state.apu.bufferPosition;
}

void GameBoy::clearAudioBuffer() {
    state.apu.bufferPosition = 0;
}

bool GameBoy::hasSRAM() const {
    return state.cartridge.ramSize > 0;
}

const char* GameBoy::getROMTitle() const {
    return state.cartridge.title;
}

bool GameBoy::loadOpcodeTable(const char* filepath) {
    return gb::opcode_parser::parse(filepath, state.opcodes);
}

// ... rest stays same ...