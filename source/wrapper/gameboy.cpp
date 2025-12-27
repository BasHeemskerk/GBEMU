#include "included/gameboy.hpp"
#include "../gb/included/gb.hpp"
#include "../gb/included/ppu.hpp"
#include "../gb/included/apu.hpp"
#include "../gb/included/joypad.hpp"
#include "../gb/included/cartridge.hpp"

GameBoy::GameBoy() : romLoaded(false){
    input.clear();
}

GameBoy::~GameBoy(){
    //cleanup if needed
}

void GameBoy::init(){
    gb::initialize();
    romLoaded = false;
    input.clear();
}

void GameBoy::reset(){
    gb::initialize();
    input.clear();
}

void GameBoy::runFrame(){
    if (!romLoaded){
        return;
    }

    updateInput();
    gb::runFrame();
}

void GameBoy::step(){
    if (!romLoaded){
        return;
    }

    updateInput();
    gb::step();
}


//rom management

bool GameBoy::loadROM(const char* filepath){
    romLoaded = gb::loadROM(filepath);
    return romLoaded;
}

bool GameBoy::isROMLoaded() const {
    return romLoaded;
}

const char* GameBoy::getROMTitle() const {
    return gb::cartridge::getTitle();
}


//input
void GameBoy::updateInput() {
    gb::joypad::buttonA = input.a;
    gb::joypad::buttonB = input.b;
    gb::joypad::buttonStart = input.start;
    gb::joypad::buttonSelect = input.select;
    gb::joypad::dpadUp = input.up;
    gb::joypad::dpadDown = input.down;
    gb::joypad::dpadLeft = input.left;
    gb::joypad::dpadRight = input.right;
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


//video output
uint8_t* GameBoy::getFramebuffer(){
    return gb::ppu::framebuffer;
}

const uint8_t* GameBoy::getFramebuffer() const {
    return gb::ppu::framebuffer;
}

bool GameBoy::isFrameReady() const {
    return gb::ppu::frameReady;
}

void GameBoy::clearFrameReady(){
    gb::ppu::frameReady = false;
}


//audio output
int16_t* GameBoy::getAudioBuffer(){
    return gb::apu::audioBuffer;
}

int GameBoy::getAudioBufferPosition() const {
    return gb::apu::bufferPosition;
}

void GameBoy::clearAudioBuffer(){
    gb::apu::bufferPosition = 0;
}


//save data
bool GameBoy::hasSRAM() const {
    return gb::cartridge::ramSize > 0;
}

bool GameBoy::saveSRAM(const char* filepath) {
    // TODO: implement
    return false;
}

bool GameBoy::loadSRAM(const char* filepath) {
    // TODO: implement
    return false;
}