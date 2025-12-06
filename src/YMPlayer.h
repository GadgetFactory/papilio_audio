/**
 * @file YMPlayer.h
 * @brief YM file player for YM2149 chip
 * 
 * Plays .ymd files (compressed YM register dumps) on the YM2149 PSG.
 * Format: 16 bytes per frame (14 YM registers + 2 padding), plays at 50Hz
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#ifndef YM_PLAYER_H
#define YM_PLAYER_H

#include <Arduino.h>
#include <LittleFS.h>
#include "YM2149.h"

class YMPlayer {
public:
    YMPlayer(YM2149& ym);
    
    bool begin();
    bool loadFile(const char* filename);
    void play();
    void stop();
    void pause();
    void resume();
    bool isPlaying() const { return _playing; }
    bool isPaused() const { return _paused; }
    
    // Call this at 50Hz to update YM registers
    void update();
    
    void setVolume(uint8_t vol);
    uint8_t getVolume() const { return _volume; }
    
private:
    YM2149& _ym;
    File _file;
    bool _playing;
    bool _paused;
    uint8_t _volume;
    
    struct YMFrame {
        uint8_t regs[14];
        uint8_t padding[2];
    };
    
    bool readFrame(YMFrame& frame);
};

#endif // YM_PLAYER_H
