#ifndef WRAPPER_GAMEBOY_HPP
#define WRAPPER_GAMEBOY_HPP

#include <cstdint>
#include <cstddef>

//gameboy screen dimensions
constexpr int GB_SCREEN_WIDTH = 160;
constexpr int GB_SCREEN_HEIGHT = 144;

//gameboy audio constant
constexpr int GB_SAMPLE_RATE = 32768;
constexpr int GB_AUDIO_BUFFER_SIZE = 2048;

class GameBoy{
    public:
        //lifecycle
        GameBoy();
        ~GameBoy();

        void init(); //initialize subsystems
        void reset(); //reset to initial state (keeps rom loaded)
        void runFrame(); //run one frame (~70224 cycles, until vblank)
        void step(); //run a single cpu step (debugging purposes)


        //rom management
        bool loadROM(const char* filepath); //load rom from file path
        bool isROMLoaded() const; //check if a rom is loaded
        const char* getROMTitle() const; //get the rom title from CR header


        //input system
        struct Input{
            bool a;
            bool b;
            bool start;
            bool select;
            bool up;
            bool down;
            bool left;
            bool right;

            void clear(){
                a = b = start = select = false;
                up = down = left = right = false;
            }

        } input;

        //pack input to byte for stuff like replay and netplay
        uint8_t getInputState() const;
        void setInputState(uint8_t state);


        //video output
        //get framebuffer (160x144 pixels, values 0-3)
        uint8_t* getFramebuffer();
        const uint8_t* getFramebuffer() const;

        //check if new frame is ready
        bool isFrameReady() const;
        void clearFrameReady();


        //audio output
        //get audio buffer (stereo inteleaved)
        int16_t* getAudioBuffer();
        int getAudioBufferPosition() const;
        void clearAudioBuffer();


        //save data (SRAM)
        bool hasSRAM() const;
        bool saveSRAM(const char* filepath);
        bool loadSRAM(const char* filepath);


        // save states (future)
                
        // size_t getStateSize() const;
        // bool saveState(void* buffer);
        // bool loadState(const void* buffer);

    private:
        bool romLoaded;

        // sync input struct to gb::joypad
        void updateInput();
};

#endif