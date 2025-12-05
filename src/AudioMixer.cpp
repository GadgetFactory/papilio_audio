/**
 * @file AudioMixer.cpp
 * @brief Audio mixer implementation
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#include "AudioMixer.h"

AudioMixer::AudioMixer(uint16_t baseAddr)
    : _baseAddr(baseAddr), _control(0), _masterVolume(255) {
}

void AudioMixer::begin() {
    reset();
    enable(true);
}

void AudioMixer::writeReg(uint8_t addr, uint8_t data) {
    wishboneWrite8(_baseAddr + addr, data);
}

uint8_t AudioMixer::readReg(uint8_t addr) {
    return wishboneRead8(_baseAddr + addr);
}

void AudioMixer::enable(bool enable) {
    if (enable) {
        _control |= MIXER_CTRL_ENABLE;
    } else {
        _control &= ~MIXER_CTRL_ENABLE;
    }
    writeReg(MIXER_REG_CONTROL, _control);
}

void AudioMixer::mute(bool mute) {
    if (mute) {
        _control |= MIXER_CTRL_MUTE;
    } else {
        _control &= ~MIXER_CTRL_MUTE;
    }
    writeReg(MIXER_REG_CONTROL, _control);
}

void AudioMixer::setMasterVolume(uint8_t volume) {
    _masterVolume = volume;
    writeReg(MIXER_REG_MASTER_VOL, volume);
}

uint8_t AudioMixer::getMasterVolume() {
    return _masterVolume;
}

void AudioMixer::enableSID(bool enable) {
    if (enable) {
        _control |= MIXER_CTRL_CH1_ENABLE;
    } else {
        _control &= ~MIXER_CTRL_CH1_ENABLE;
    }
    writeReg(MIXER_REG_CONTROL, _control);
}

void AudioMixer::enableYM2149(bool enable) {
    if (enable) {
        _control |= MIXER_CTRL_CH2_ENABLE;
    } else {
        _control &= ~MIXER_CTRL_CH2_ENABLE;
    }
    writeReg(MIXER_REG_CONTROL, _control);
}

void AudioMixer::enablePOKEY(bool enable) {
    if (enable) {
        _control |= MIXER_CTRL_CH3_ENABLE;
    } else {
        _control &= ~MIXER_CTRL_CH3_ENABLE;
    }
    writeReg(MIXER_REG_CONTROL, _control);
}

void AudioMixer::setSIDVolume(uint8_t volume) {
    writeReg(MIXER_REG_CH1_VOL, volume);
}

void AudioMixer::setYM2149Volume(uint8_t volume) {
    writeReg(MIXER_REG_CH2_VOL, volume);
}

void AudioMixer::setPOKEYVolume(uint8_t volume) {
    writeReg(MIXER_REG_CH3_VOL, volume);
}

void AudioMixer::reset() {
    _control = MIXER_CTRL_ENABLE | MIXER_CTRL_CH1_ENABLE | 
               MIXER_CTRL_CH2_ENABLE | MIXER_CTRL_CH3_ENABLE;
    _masterVolume = 255;
    
    writeReg(MIXER_REG_CONTROL, _control);
    writeReg(MIXER_REG_MASTER_VOL, _masterVolume);
    writeReg(MIXER_REG_CH1_VOL, 255);  // SID default volume
    writeReg(MIXER_REG_CH2_VOL, 255);  // YM2149 default volume
    writeReg(MIXER_REG_CH3_VOL, 255);  // POKEY default volume
}
