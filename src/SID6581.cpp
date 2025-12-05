/**
 * @file SID6581.cpp
 * @brief SID 6581 (Commodore 64) sound chip emulation implementation
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#include "SID6581.h"

// MIDI note to SID frequency conversion table
const uint16_t SIDVoice::MIDI2freq[129] = {
    // 0-7
    291, 291, 291, 291, 291, 291, 291, 291,
    // 8-15
    291, 291, 291, 291, 291, 291, 308, 326,
    // 16-23
    346, 366, 388, 411, 435, 461, 489, 518,
    // 24-31
    549, 581, 616, 652, 691, 732, 776, 822,
    // 32-39
    871, 923, 978, 1036, 1097, 1163, 1232, 1305,
    // 40-47
    1383, 1465, 1552, 1644, 1742, 1845, 1955, 2071,
    // 48-55
    2195, 2325, 2463, 2610, 2765, 2930, 3104, 3288,
    // 56-63
    3484, 3691, 3910, 4143, 4389, 4650, 4927, 5220,
    // 64-71
    5530, 5859, 6207, 6577, 6968, 7382, 7821, 8286,
    // 72-79
    8779, 9301, 9854, 10440, 11060, 11718, 12415, 13153,
    // 80-87
    13935, 14764, 15642, 16572, 17557, 18601, 19709, 20897,
    // 88-95
    22121, 23436, 24830, 26306, 27871, 29528, 31234, 33144,
    // 96-103
    35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613,
    // 104-111
    55741, 59056, 62567, 65535, 65535, 65535, 65535, 65535,
    // 112-119
    65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
    // 120-127
    65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
    // 128 (note off)
    0
};

// ============================================================================
// SIDVoice Implementation
// ============================================================================

SIDVoice::SIDVoice() : _baseAddr(0), _currentFreq(0),
                       _controlReg(0), _attackDecay(0), _sustainRelease(0) {
}

void SIDVoice::begin(uint16_t baseAddr) {
    _baseAddr = baseAddr;
    reset();
}

void SIDVoice::writeReg(uint8_t offset, uint8_t value) {
    wishboneWrite16(_baseAddr + offset, value);
}

void SIDVoice::updateControlReg() {
    writeReg(SID_VOICE_CONTROL, _controlReg);
}

void SIDVoice::updateADSR() {
    writeReg(SID_VOICE_ATTACK_DECAY, _attackDecay);
    writeReg(SID_VOICE_SUSTAIN_RELEASE, _sustainRelease);
}

void SIDVoice::setNote(uint8_t note, bool active) {
    if (note > 128) note = 128;
    setFreq(MIDI2freq[note]);
    setGate(active);
}

void SIDVoice::setFreq(uint16_t freq) {
    _currentFreq = freq;
    writeReg(SID_VOICE_FREQ_LO, freq & 0xFF);
    writeReg(SID_VOICE_FREQ_HI, (freq >> 8) & 0xFF);
}

uint16_t SIDVoice::getCurrentFreq() {
    return _currentFreq;
}

void SIDVoice::setPulseWidth(uint16_t pw) {
    writeReg(SID_VOICE_PW_LO, pw & 0xFF);
    writeReg(SID_VOICE_PW_HI, (pw >> 8) & 0x0F);
}

void SIDVoice::setPWLo(uint8_t pw) {
    writeReg(SID_VOICE_PW_LO, pw);
}

void SIDVoice::setPWHi(uint8_t pw) {
    writeReg(SID_VOICE_PW_HI, pw & 0x0F);
}

void SIDVoice::setGate(bool active) {
    if (active) {
        _controlReg |= SID_CTRL_GATE;
    } else {
        _controlReg &= ~SID_CTRL_GATE;
    }
    updateControlReg();
}

void SIDVoice::setSync(bool active) {
    if (active) {
        _controlReg |= SID_CTRL_SYNC;
    } else {
        _controlReg &= ~SID_CTRL_SYNC;
    }
    updateControlReg();
}

void SIDVoice::setRingMod(bool active) {
    if (active) {
        _controlReg |= SID_CTRL_RING_MOD;
    } else {
        _controlReg &= ~SID_CTRL_RING_MOD;
    }
    updateControlReg();
}

void SIDVoice::setTest(bool active) {
    if (active) {
        _controlReg |= SID_CTRL_TEST;
    } else {
        _controlReg &= ~SID_CTRL_TEST;
    }
    updateControlReg();
}

void SIDVoice::setTriangle(bool active) {
    if (active) {
        _controlReg |= SID_CTRL_TRIANGLE;
    } else {
        _controlReg &= ~SID_CTRL_TRIANGLE;
    }
    updateControlReg();
}

void SIDVoice::setSawtooth(bool active) {
    if (active) {
        _controlReg |= SID_CTRL_SAWTOOTH;
    } else {
        _controlReg &= ~SID_CTRL_SAWTOOTH;
    }
    updateControlReg();
}

void SIDVoice::setSquare(bool active, uint16_t pwm) {
    if (active) {
        _controlReg |= SID_CTRL_SQUARE;
        setPulseWidth(pwm);
    } else {
        _controlReg &= ~SID_CTRL_SQUARE;
    }
    updateControlReg();
}

void SIDVoice::setNoise(bool active) {
    if (active) {
        _controlReg |= SID_CTRL_NOISE;
    } else {
        _controlReg &= ~SID_CTRL_NOISE;
    }
    updateControlReg();
}

void SIDVoice::setEnvelopeAttack(uint8_t rate) {
    _attackDecay = (_attackDecay & 0x0F) | ((rate & 0x0F) << 4);
    updateADSR();
}

void SIDVoice::setEnvelopeDecay(uint8_t rate) {
    _attackDecay = (_attackDecay & 0xF0) | (rate & 0x0F);
    updateADSR();
}

void SIDVoice::setEnvelopeSustain(uint8_t level) {
    _sustainRelease = (_sustainRelease & 0x0F) | ((level & 0x0F) << 4);
    updateADSR();
}

void SIDVoice::setEnvelopeRelease(uint8_t rate) {
    _sustainRelease = (_sustainRelease & 0xF0) | (rate & 0x0F);
    updateADSR();
}

void SIDVoice::setInstrument(const char* name, uint8_t attack, uint8_t decay,
                             uint8_t sustain, uint8_t release,
                             bool noise, bool square, bool sawtooth, bool triangle,
                             uint16_t pwm) {
    setEnvelopeAttack(attack);
    setEnvelopeDecay(decay);
    setEnvelopeSustain(sustain);
    setEnvelopeRelease(release);
    
    // Clear all waveform bits first
    _controlReg &= ~(SID_CTRL_NOISE | SID_CTRL_SQUARE | SID_CTRL_SAWTOOTH | SID_CTRL_TRIANGLE);
    
    if (noise) _controlReg |= SID_CTRL_NOISE;
    if (square) {
        _controlReg |= SID_CTRL_SQUARE;
        setPulseWidth(pwm);
    }
    if (sawtooth) _controlReg |= SID_CTRL_SAWTOOTH;
    if (triangle) _controlReg |= SID_CTRL_TRIANGLE;
    
    updateControlReg();
}

void SIDVoice::reset() {
    _currentFreq = 0;
    _controlReg = 0;
    _attackDecay = 0;
    _sustainRelease = 0;
    
    writeReg(SID_VOICE_FREQ_LO, 0);
    writeReg(SID_VOICE_FREQ_HI, 0);
    writeReg(SID_VOICE_PW_LO, 0);
    writeReg(SID_VOICE_PW_HI, 0);
    writeReg(SID_VOICE_CONTROL, 0);
    writeReg(SID_VOICE_ATTACK_DECAY, 0);
    writeReg(SID_VOICE_SUSTAIN_RELEASE, 0);
}

// ============================================================================
// SID6581 Implementation
// ============================================================================

SID6581::SID6581(uint16_t baseAddr) 
    : _baseAddr(baseAddr), _modeVolume(0), _resFilt(0) {
}

void SID6581::begin() {
    V1.begin(_baseAddr + SID_VOICE1_BASE);
    V2.begin(_baseAddr + SID_VOICE2_BASE);
    V3.begin(_baseAddr + SID_VOICE3_BASE);
    reset();
}

void SID6581::writeReg(uint8_t addr, uint8_t data) {
    wishboneWrite16(_baseAddr + addr, data);
}

uint8_t SID6581::readReg(uint8_t addr) {
    return wishboneRead16(_baseAddr + addr);
}

void SID6581::setVolume(uint8_t volume) {
    _modeVolume = (_modeVolume & 0xF0) | (volume & 0x0F);
    writeReg(SID_FILTER_MODE_VOL, _modeVolume);
}

void SID6581::setFilterCutoff(uint16_t freq) {
    writeReg(SID_FILTER_FC_LO, freq & 0x07);  // Only 3 LSB used
    writeReg(SID_FILTER_FC_HI, (freq >> 3) & 0xFF);
}

void SID6581::setFilterResonance(uint8_t resonance) {
    _resFilt = (_resFilt & 0x0F) | ((resonance & 0x0F) << 4);
    writeReg(SID_FILTER_RES_FILT, _resFilt);
}

void SID6581::setFilterEnable(uint8_t voice, bool enable) {
    uint8_t bit = (1 << (voice - 1)) & 0x07;
    if (enable) {
        _resFilt |= bit;
    } else {
        _resFilt &= ~bit;
    }
    writeReg(SID_FILTER_RES_FILT, _resFilt);
}

void SID6581::setFilterMode(bool lowpass, bool bandpass, bool highpass) {
    _modeVolume &= 0x0F;  // Keep volume
    if (lowpass)  _modeVolume |= 0x10;
    if (bandpass) _modeVolume |= 0x20;
    if (highpass) _modeVolume |= 0x40;
    writeReg(SID_FILTER_MODE_VOL, _modeVolume);
}

void SID6581::reset() {
    V1.reset();
    V2.reset();
    V3.reset();
    
    _modeVolume = 0;
    _resFilt = 0;
    
    writeReg(SID_FILTER_FC_LO, 0);
    writeReg(SID_FILTER_FC_HI, 0);
    writeReg(SID_FILTER_RES_FILT, 0);
    writeReg(SID_FILTER_MODE_VOL, 0);
}
