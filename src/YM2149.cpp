/**
 * @file YM2149.cpp
 * @brief YM2149 (AY-3-8910) sound chip emulation implementation
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#include "YM2149.h"

// Shared mixer register state (static for all voices)
static uint8_t s_ymMixer = 0x3F;  // All disabled by default (bits are inverted)

// MIDI note to YM2149 frequency divider table
// YM2149 divides 2MHz clock, freq = 2000000 / (16 * tp) where tp = register value
const uint16_t YMVoice::MIDI2freq[129] = {
    // 0-7
    15289, 14431, 13621, 12856, 12135, 11454, 10811, 10204,
    // 8-15
    9631, 9091, 8581, 8099, 7645, 7215, 6810, 6428,
    // 16-23
    6067, 5727, 5405, 5102, 4816, 4545, 4290, 4050,
    // 24-31
    3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551,
    // 32-39
    2408, 2273, 2145, 2025, 1911, 1804, 1703, 1607,
    // 40-47
    1517, 1432, 1351, 1276, 1204, 1136, 1073, 1012,
    // 48-55
    956, 902, 851, 804, 758, 716, 676, 638,
    // 56-63
    602, 568, 536, 506, 478, 451, 426, 402,
    // 64-71
    379, 358, 338, 319, 301, 284, 268, 253,
    // 72-79
    239, 225, 213, 201, 190, 179, 169, 159,
    // 80-87
    150, 142, 134, 127, 119, 113, 106, 100,
    // 88-95
    95, 89, 84, 80, 75, 71, 67, 63,
    // 96-103
    60, 56, 53, 50, 47, 45, 42, 40,
    // 104-111
    38, 36, 34, 32, 30, 28, 27, 25,
    // 112-119
    24, 22, 21, 20, 19, 18, 17, 16,
    // 120-127
    15, 14, 13, 13, 12, 11, 11, 10,
    // 128 (note off)
    0
};

// ============================================================================
// YMVoice Implementation
// ============================================================================

YMVoice::YMVoice() : _baseAddr(0), _freqAddr(0), _levelAddr(0),
                     _voiceNum(0), _currentFreq(0), _level(0), _useEnvelope(false) {
}

void YMVoice::begin(uint16_t baseAddr, uint8_t freqAddr,
                    uint8_t levelAddr, uint8_t voiceNum) {
    _baseAddr = baseAddr;
    _freqAddr = freqAddr;
    _levelAddr = levelAddr;
    _voiceNum = voiceNum;
    
    // Set the bit positions for this voice in the mixer register
    _toneBit = (1 << voiceNum);
    _noiseBit = (1 << (voiceNum + 3));
    
    reset();
}

void YMVoice::writeReg(uint8_t addr, uint8_t value) {
    wishboneWrite16(_baseAddr + addr, value);
}

uint8_t YMVoice::readReg(uint8_t addr) {
    return wishboneRead16(_baseAddr + addr);
}

void YMVoice::updateLevel() {
    uint8_t levelReg = _level & 0x0F;
    if (_useEnvelope) {
        levelReg |= YM_LEVEL_MODE_ENV;
    }
    writeReg(_levelAddr, levelReg);
}

void YMVoice::updateMixer() {
    writeReg(YM_REG_MIXER, s_ymMixer);
}

void YMVoice::setNote(uint8_t note, bool active) {
    if (note > 128) note = 128;
    setFreq(MIDI2freq[note]);
}

void YMVoice::setFreq(uint16_t freq) {
    _currentFreq = freq;
    writeReg(_freqAddr, freq & 0xFF);
    writeReg(_freqAddr + 1, (freq >> 8) & 0x0F);
}

uint16_t YMVoice::getCurrentFreq() {
    return _currentFreq;
}

void YMVoice::setVolume(uint8_t volume) {
    _level = volume & 0x0F;
    updateLevel();
}

uint8_t YMVoice::getVolume() {
    return _level;
}

void YMVoice::setEnvelope(bool active) {
    _useEnvelope = active;
    updateLevel();
}

void YMVoice::setTone(bool active) {
    // Note: In YM2149, 0 = enabled, 1 = disabled
    if (active) {
        s_ymMixer &= ~_toneBit;  // Clear bit to enable
    } else {
        s_ymMixer |= _toneBit;   // Set bit to disable
    }
    updateMixer();
}

void YMVoice::setNoise(bool active) {
    // Note: In YM2149, 0 = enabled, 1 = disabled
    if (active) {
        s_ymMixer &= ~_noiseBit;  // Clear bit to enable
    } else {
        s_ymMixer |= _noiseBit;   // Set bit to disable
    }
    updateMixer();
}

void YMVoice::reset() {
    _currentFreq = 0;
    _level = 0;
    _useEnvelope = false;
    
    writeReg(_freqAddr, 0);
    writeReg(_freqAddr + 1, 0);
    updateLevel();
    
    // Disable this voice in mixer
    s_ymMixer |= _toneBit | _noiseBit;
    updateMixer();
}

// ============================================================================
// YM2149 Implementation
// ============================================================================

YM2149::YM2149(uint16_t baseAddr)
    : _baseAddr(baseAddr), _mixer(0x3F) {
}

void YM2149::begin() {
    s_ymMixer = 0x3F;  // Reset shared mixer state
    
    V1.begin(_baseAddr, YM_REG_FREQ_A_LO, YM_REG_LEVEL_A, 0);
    V2.begin(_baseAddr, YM_REG_FREQ_B_LO, YM_REG_LEVEL_B, 1);
    V3.begin(_baseAddr, YM_REG_FREQ_C_LO, YM_REG_LEVEL_C, 2);
    
    reset();
}

void YM2149::writeReg(uint8_t addr, uint8_t data) {
    wishboneWrite16(_baseAddr + addr, data);
}

uint8_t YM2149::readReg(uint8_t addr) {
    return wishboneRead16(_baseAddr + addr);
}

void YM2149::setNoiseFrequency(uint8_t freq) {
    writeReg(YM_REG_NOISE_FREQ, freq & 0x1F);
}

void YM2149::setEnvelopeFrequency(uint16_t freq) {
    writeReg(YM_REG_ENV_FREQ_LO, freq & 0xFF);
    writeReg(YM_REG_ENV_FREQ_HI, (freq >> 8) & 0xFF);
}

void YM2149::setEnvelopeShape(bool cont, bool att, bool alt, bool hold) {
    uint8_t shape = 0;
    if (hold) shape |= YM_ENV_HOLD;
    if (alt)  shape |= YM_ENV_ALTERNATE;
    if (att)  shape |= YM_ENV_ATTACK;
    if (cont) shape |= YM_ENV_CONTINUE;
    writeReg(YM_REG_ENV_SHAPE, shape);
}

void YM2149::reset() {
    V1.reset();
    V2.reset();
    V3.reset();
    
    s_ymMixer = 0x3F;  // All disabled
    writeReg(YM_REG_MIXER, s_ymMixer);
    writeReg(YM_REG_NOISE_FREQ, 0);
    writeReg(YM_REG_ENV_FREQ_LO, 0);
    writeReg(YM_REG_ENV_FREQ_HI, 0);
    writeReg(YM_REG_ENV_SHAPE, 0);
}
