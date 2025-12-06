/**
 * @file YMPlayer.cpp
 * @brief YM file player implementation
 */

#include "YMPlayer.h"

YMPlayer::YMPlayer(YM2149& ym) 
    : _ym(ym), _playing(false), _paused(false), _volume(11) {
}

bool YMPlayer::begin() {
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS");
        return false;
    }
    return true;
}

bool YMPlayer::loadFile(const char* filename) {
    stop();
    
    if (_file) {
        _file.close();
    }
    
    _file = LittleFS.open(filename, "r");
    if (!_file) {
        Serial.print("Failed to open: ");
        Serial.println(filename);
        return false;
    }
    
    Serial.print("Loaded: ");
    Serial.print(filename);
    Serial.print(" (");
    Serial.print(_file.size());
    Serial.println(" bytes)");
    
    return true;
}

void YMPlayer::play() {
    if (!_file) {
        Serial.println("No file loaded");
        return;
    }
    
    _file.seek(0);
    _playing = true;
    _paused = false;
    
    // Reset YM chip
    _ym.reset();
    _ym.V1.setVolume(_volume);
    _ym.V2.setVolume(_volume);
    _ym.V3.setVolume(_volume);
}

void YMPlayer::stop() {
    _playing = false;
    _paused = false;
    
    // Silence the YM chip
    _ym.V1.setTone(false);
    _ym.V2.setTone(false);
    _ym.V3.setTone(false);
}

void YMPlayer::pause() {
    _paused = true;
}

void YMPlayer::resume() {
    _paused = false;
}

void YMPlayer::setVolume(uint8_t vol) {
    _volume = constrain(vol, 0, 15);
    if (_playing) {
        _ym.V1.setVolume(_volume);
        _ym.V2.setVolume(_volume);
        _ym.V3.setVolume(_volume);
    }
}

bool YMPlayer::readFrame(YMFrame& frame) {
    if (!_file || !_playing) {
        return false;
    }
    
    size_t bytesRead = _file.read((uint8_t*)&frame, sizeof(YMFrame));
    
    // Loop back to start if we hit EOF
    if (bytesRead != sizeof(YMFrame)) {
        _file.seek(0);
        bytesRead = _file.read((uint8_t*)&frame, sizeof(YMFrame));
        if (bytesRead != sizeof(YMFrame)) {
            Serial.println("Failed to read frame");
            return false;
        }
    }
    
    return true;
}

void YMPlayer::update() {
    if (!_playing || _paused) {
        return;
    }
    
    YMFrame frame;
    if (readFrame(frame)) {
        // Apply volume adjustment to amplitude registers (8, 9, 10)
        frame.regs[8] = constrain(frame.regs[8] - (15 - _volume), 0, 15);
        frame.regs[9] = constrain(frame.regs[9] - (15 - _volume), 0, 15);
        frame.regs[10] = constrain(frame.regs[10] - (15 - _volume), 0, 15);
        
        // Write all 14 registers to the YM2149
        for (int i = 0; i < 14; i++) {
            _ym.writeReg(i, frame.regs[i]);
        }
    }
}
