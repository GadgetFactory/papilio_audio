/**
 * @file AudioMixer.h
 * @brief Audio mixer for combining multiple sound chip outputs
 * 
 * The AudioMixer allows mixing outputs from multiple sound chips
 * (SID, YM2149, POKEY) into a single audio output via sigma-delta DAC.
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <Arduino.h>
#include "WishboneSPI.h"

// Audio Mixer Register addresses
#define MIXER_REG_CONTROL       0x00    // Control register
#define MIXER_REG_MASTER_VOL    0x01    // Master volume
#define MIXER_REG_CH1_VOL       0x02    // Channel 1 (SID) volume
#define MIXER_REG_CH2_VOL       0x03    // Channel 2 (YM2149) volume
#define MIXER_REG_CH3_VOL       0x04    // Channel 3 (POKEY) volume
#define MIXER_REG_STATUS        0x05    // Status register

// Control register bits
#define MIXER_CTRL_ENABLE       0x01    // Enable mixer output
#define MIXER_CTRL_CH1_ENABLE   0x02    // Enable channel 1 (SID)
#define MIXER_CTRL_CH2_ENABLE   0x04    // Enable channel 2 (YM2149)
#define MIXER_CTRL_CH3_ENABLE   0x08    // Enable channel 3 (POKEY)
#define MIXER_CTRL_MUTE         0x80    // Mute all output

/**
 * @class AudioMixer
 * @brief Controls audio mixing from multiple sound sources
 */
class AudioMixer {
public:
    /**
     * @brief Constructor
     * @param baseAddr Base address for mixer (default 0x70)
     */
    AudioMixer(uint16_t baseAddr = 0x70);
    
    /**
     * @brief Initialize the mixer
     */
    void begin();
    
    /**
     * @brief Enable the mixer output
     * @param enable True to enable
     */
    void enable(bool enable = true);
    
    /**
     * @brief Mute all audio output
     * @param mute True to mute
     */
    void mute(bool mute = true);
    
    /**
     * @brief Set master volume
     * @param volume Volume level (0-255)
     */
    void setMasterVolume(uint8_t volume);
    
    /**
     * @brief Get master volume
     * @return Current master volume
     */
    uint8_t getMasterVolume();
    
    /**
     * @brief Enable/disable SID input
     * @param enable True to enable
     */
    void enableSID(bool enable = true);
    
    /**
     * @brief Enable/disable YM2149 input
     * @param enable True to enable
     */
    void enableYM2149(bool enable = true);
    
    /**
     * @brief Enable/disable POKEY input
     * @param enable True to enable
     */
    void enablePOKEY(bool enable = true);
    
    /**
     * @brief Set SID channel volume
     * @param volume Volume level (0-255)
     */
    void setSIDVolume(uint8_t volume);
    
    /**
     * @brief Set YM2149 channel volume
     * @param volume Volume level (0-255)
     */
    void setYM2149Volume(uint8_t volume);
    
    /**
     * @brief Set POKEY channel volume
     * @param volume Volume level (0-255)
     */
    void setPOKEYVolume(uint8_t volume);
    
    /**
     * @brief Write to a mixer register
     * @param addr Register address
     * @param data Data to write
     */
    void writeReg(uint8_t addr, uint8_t data);
    
    /**
     * @brief Read from a mixer register
     * @param addr Register address
     * @return Register value
     */
    uint8_t readReg(uint8_t addr);
    
    /**
     * @brief Reset mixer to default state
     */
    void reset();
    
private:
    uint16_t _baseAddr;
    uint8_t _control;
    uint8_t _masterVolume;
};

#endif // AUDIO_MIXER_H
