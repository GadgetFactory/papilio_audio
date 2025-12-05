/**
 * @file SIDPlayer.h
 * @brief SID file player for the Papilio Arcade
 * 
 * Plays .sid (Commodore 64 SID music) files using a 6502 emulator
 * and the SID 6581 gateware.
 * 
 * Based on TinySID by:
 * - Tammo Hinrichs - 6510 and SID routines
 * - Rainer Sinsch - PSP TinySID
 * - Alvaro Lopes - ZPUino adaptation
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#ifndef SID_PLAYER_H
#define SID_PLAYER_H

#include <Arduino.h>
#include "SID6581.h"

// CPU Flags
#define FLAG_N 128
#define FLAG_V 64
#define FLAG_B 16
#define FLAG_D 8
#define FLAG_I 4
#define FLAG_Z 2
#define FLAG_C 1

/**
 * @class SIDPlayer
 * @brief Plays .sid files using 6502 CPU emulation
 */
class SIDPlayer {
public:
    /**
     * @brief Constructor
     * @param sid Pointer to SID6581 instance
     */
    SIDPlayer(SID6581* sid);
    
    /**
     * @brief Initialize the player
     */
    void begin();
    
    /**
     * @brief Load a SID file from memory
     * @param data Pointer to SID file data
     * @param length Length of data in bytes
     * @param subSong Sub-song number to play (0 = default)
     * @return true if loaded successfully
     */
    bool loadFromMemory(const uint8_t* data, size_t length, uint8_t subSong = 0);
    
    /**
     * @brief Load a SID file from SPIFFS/LittleFS
     * @param filename Path to .sid file
     * @param subSong Sub-song number to play (0 = default)
     * @return true if loaded successfully
     */
    bool loadFile(const char* filename, uint8_t subSong = 0);
    
    /**
     * @brief Start/stop playback
     * @param play true to start, false to stop
     */
    void play(bool play);
    
    /**
     * @brief Check if currently playing
     * @return true if playing
     */
    bool isPlaying();
    
    /**
     * @brief Call this at 50Hz (PAL) or 60Hz (NTSC) from timer interrupt
     * Called from ISR context - runs the play routine
     */
    void timerCallback();
    
    /**
     * @brief Call this from main loop to process audio
     * Runs the 6502 emulator
     */
    void update();
    
    /**
     * @brief Get song title from SID file
     * @return Song title string
     */
    const char* getTitle();
    
    /**
     * @brief Get author from SID file
     * @return Author string
     */
    const char* getAuthor();
    
    /**
     * @brief Get copyright from SID file
     * @return Copyright string
     */
    const char* getCopyright();
    
    /**
     * @brief Get number of sub-songs
     * @return Number of sub-songs
     */
    uint8_t getNumSongs();
    
    /**
     * @brief Get current sub-song
     * @return Current sub-song number
     */
    uint8_t getCurrentSong();
    
    /**
     * @brief Play next sub-song
     */
    void nextSong();
    
    /**
     * @brief Play previous sub-song
     */
    void prevSong();

private:
    SID6581* _sid;
    bool _playing;
    bool _fileLoaded;
    volatile bool _timerTick;
    
    // SID file header info
    uint16_t _loadAddr;
    uint16_t _initAddr;
    uint16_t _playAddr;
    uint8_t _numSongs;
    uint8_t _currentSong;
    char _title[33];
    char _author[33];
    char _copyright[33];
    
    // 6502 CPU state
    uint8_t _memory[65536];
    uint8_t _a, _x, _y, _s, _p;
    uint16_t _pc;
    uint32_t _cycles;
    
    // CPU emulation
    void cpuReset();
    void cpuJsr(uint16_t addr, uint8_t acc);
    bool cpuStep();
    
    uint8_t getMem(uint16_t addr);
    void setMem(uint16_t addr, uint8_t value);
    
    uint8_t getAddr(uint8_t mode);
    void setAddr(uint8_t mode, uint8_t val);
    void putAddr(uint8_t mode, uint8_t val);
    void branch(bool condition);
    
    // Parse SID file header
    bool parseSIDHeader(const uint8_t* data, size_t length);
};

#endif // SID_PLAYER_H
