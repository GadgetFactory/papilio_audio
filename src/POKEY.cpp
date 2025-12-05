/**
 * @file POKEY.cpp
 * @brief POKEY (Atari) sound chip emulation implementation
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#include "POKEY.h"

// ============================================================================
// POKEYChannel Implementation
// ============================================================================

POKEYChannel::POKEYChannel() : _baseAddr(0), _freqAddr(0),
                               _ctrlAddr(0), _freq(0), _ctrl(0) {
}

void POKEYChannel::begin(uint16_t baseAddr, uint8_t freqAddr, uint8_t ctrlAddr) {
    _baseAddr = baseAddr;
    _freqAddr = freqAddr;
    _ctrlAddr = ctrlAddr;
    reset();
}

void POKEYChannel::writeReg(uint8_t addr, uint8_t value) {
    wishboneWrite8(_baseAddr + addr, value);
}

void POKEYChannel::updateCtrl() {
    writeReg(_ctrlAddr, _ctrl);
}

void POKEYChannel::setFrequency(uint8_t freq) {
    _freq = freq;
    writeReg(_freqAddr, freq);
}

uint8_t POKEYChannel::getFrequency() {
    return _freq;
}

void POKEYChannel::setVolume(uint8_t volume) {
    _ctrl = (_ctrl & 0xF0) | (volume & 0x0F);
    updateCtrl();
}

uint8_t POKEYChannel::getVolume() {
    return _ctrl & 0x0F;
}

void POKEYChannel::setDistortion(uint8_t dist) {
    _ctrl = (_ctrl & 0x0F) | (dist & 0xF0);
    updateCtrl();
}

void POKEYChannel::setVolumeOnly(bool active) {
    if (active) {
        _ctrl |= 0x10;  // Set volume-only bit
    } else {
        _ctrl &= ~0x10;
    }
    updateCtrl();
}

void POKEYChannel::reset() {
    _freq = 0;
    _ctrl = 0;
    writeReg(_freqAddr, 0);
    writeReg(_ctrlAddr, 0);
}

// ============================================================================
// POKEY Implementation
// ============================================================================

POKEY::POKEY(uint16_t baseAddr)
    : _baseAddr(baseAddr), _audctl(0) {
}

void POKEY::begin() {
    CH1.begin(_baseAddr, POKEY_REG_AUDF1, POKEY_REG_AUDC1);
    CH2.begin(_baseAddr, POKEY_REG_AUDF2, POKEY_REG_AUDC2);
    CH3.begin(_baseAddr, POKEY_REG_AUDF3, POKEY_REG_AUDC3);
    CH4.begin(_baseAddr, POKEY_REG_AUDF4, POKEY_REG_AUDC4);
    reset();
}

void POKEY::writeReg(uint8_t addr, uint8_t data) {
    wishboneWrite8(_baseAddr + addr, data);
}

uint8_t POKEY::readReg(uint8_t addr) {
    return wishboneRead8(_baseAddr + addr);
}

void POKEY::setAUDCTL(uint8_t value) {
    _audctl = value;
    writeReg(POKEY_REG_AUDCTL, _audctl);
}

uint8_t POKEY::getAUDCTL() {
    return _audctl;
}

void POKEY::setPoly9(bool enable) {
    if (enable) {
        _audctl |= POKEY_AUDCTL_POLY9;
    } else {
        _audctl &= ~POKEY_AUDCTL_POLY9;
    }
    writeReg(POKEY_REG_AUDCTL, _audctl);
}

void POKEY::set15kHz(bool enable) {
    if (enable) {
        _audctl |= POKEY_AUDCTL_15KHZ;
    } else {
        _audctl &= ~POKEY_AUDCTL_15KHZ;
    }
    writeReg(POKEY_REG_AUDCTL, _audctl);
}

void POKEY::joinChannels12(bool enable) {
    if (enable) {
        _audctl |= POKEY_AUDCTL_CH12_JOIN;
    } else {
        _audctl &= ~POKEY_AUDCTL_CH12_JOIN;
    }
    writeReg(POKEY_REG_AUDCTL, _audctl);
}

void POKEY::joinChannels34(bool enable) {
    if (enable) {
        _audctl |= POKEY_AUDCTL_CH34_JOIN;
    } else {
        _audctl &= ~POKEY_AUDCTL_CH34_JOIN;
    }
    writeReg(POKEY_REG_AUDCTL, _audctl);
}

void POKEY::reset() {
    CH1.reset();
    CH2.reset();
    CH3.reset();
    CH4.reset();
    _audctl = 0;
    writeReg(POKEY_REG_AUDCTL, 0);
}
