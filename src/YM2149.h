/**
 * @file YM2149.h
 * @brief YM2149 (AY-3-8910) sound chip emulation API
 * 
 * The YM2149 is a 3-voice Programmable Sound Generator (PSG) that was
 * widely used in computers and arcade games including Atari ST, 
 * ZX Spectrum 128, and MSX computers.
 * 
 * Features:
 * - 3 square wave tone generators
 * - 1 noise generator
 * - Envelope generator
 * - Mixer control
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#ifndef YM2149_H
#define YM2149_H

#include <Arduino.h>
#include "WishboneSPI.h"

// YM2149 Register addresses
#define YM_REG_FREQ_A_LO        0x00
#define YM_REG_FREQ_A_HI        0x01
#define YM_REG_FREQ_B_LO        0x02
#define YM_REG_FREQ_B_HI        0x03
#define YM_REG_FREQ_C_LO        0x04
#define YM_REG_FREQ_C_HI        0x05
#define YM_REG_NOISE_FREQ       0x06
#define YM_REG_MIXER            0x07
#define YM_REG_LEVEL_A          0x08
#define YM_REG_LEVEL_B          0x09
#define YM_REG_LEVEL_C          0x0A
#define YM_REG_ENV_FREQ_LO      0x0B
#define YM_REG_ENV_FREQ_HI      0x0C
#define YM_REG_ENV_SHAPE        0x0D

// Mixer register bits
#define YM_MIXER_TONE_A         0x01
#define YM_MIXER_TONE_B         0x02
#define YM_MIXER_TONE_C         0x04
#define YM_MIXER_NOISE_A        0x08
#define YM_MIXER_NOISE_B        0x10
#define YM_MIXER_NOISE_C        0x20

// Level register bits
#define YM_LEVEL_MODE_ENV       0x10  // Use envelope if set

// Envelope shape bits
#define YM_ENV_HOLD             0x01
#define YM_ENV_ALTERNATE        0x02
#define YM_ENV_ATTACK           0x04
#define YM_ENV_CONTINUE         0x08

/**
 * @class YMVoice
 * @brief Represents a single YM2149 voice channel
 */
class YMVoice {
public:
    YMVoice();
    
    /**
     * @brief Initialize the voice
     * @param baseAddr Base address for this chip
     * @param freqAddr Register address for frequency
     * @param levelAddr Register address for level
     * @param voiceNum Voice number (0, 1, or 2)
     */
    void begin(uint16_t baseAddr, uint8_t freqAddr, 
               uint8_t levelAddr, uint8_t voiceNum);
    
    /**
     * @brief Set frequency from MIDI note
     * @param note MIDI note number (0-127)
     * @param active Unused (for API compatibility)
     */
    void setNote(uint8_t note, bool active = true);
    
    /**
     * @brief Set raw frequency value
     * @param freq 12-bit frequency divider
     */
    void setFreq(uint16_t freq);
    
    /**
     * @brief Get current frequency
     * @return Current frequency value
     */
    uint16_t getCurrentFreq();
    
    /**
     * @brief Set voice volume
     * @param volume Volume level (0-15)
     */
    void setVolume(uint8_t volume);
    
    /**
     * @brief Get current volume
     * @return Current volume level
     */
    uint8_t getVolume();
    
    /**
     * @brief Enable/disable envelope mode for this voice
     * @param active True to use envelope, false for fixed volume
     */
    void setEnvelope(bool active);
    
    /**
     * @brief Enable/disable tone output for this voice
     * @param active True to enable tone
     */
    void setTone(bool active);
    
    /**
     * @brief Enable/disable noise output for this voice
     * @param active True to enable noise
     */
    void setNoise(bool active);
    
    /**
     * @brief Reset voice to default state
     */
    void reset();
    
private:
    uint16_t _baseAddr;
    uint8_t _freqAddr;
    uint8_t _levelAddr;
    uint8_t _voiceNum;
    uint8_t _toneBit;
    uint8_t _noiseBit;
    uint16_t _currentFreq;
    uint8_t _level;
    bool _useEnvelope;
    
    void writeReg(uint8_t addr, uint8_t value);
    uint8_t readReg(uint8_t addr);
    void updateLevel();
    void updateMixer();
    
    // MIDI to YM frequency conversion table
    static const uint16_t MIDI2freq[129];
};

/**
 * @class YM2149
 * @brief Complete YM2149 chip with 3 voices and global controls
 */
class YM2149 {
public:
    YMVoice V1;  ///< Voice A
    YMVoice V2;  ///< Voice B
    YMVoice V3;  ///< Voice C
    
    /**
     * @brief Constructor
     * @param baseAddr Base address for YM2149 (default 0x50)
     */
    YM2149(uint16_t baseAddr = 0x50);
    
    /**
     * @brief Initialize the YM2149
     */
    void begin();
    
    /**
     * @brief Write to a YM2149 register
     * @param addr Register address
     * @param data Data to write
     */
    void writeReg(uint8_t addr, uint8_t data);
    
    /**
     * @brief Read from a YM2149 register
     * @param addr Register address
     * @return Register value
     */
    uint8_t readReg(uint8_t addr);
    
    /**
     * @brief Set noise generator frequency
     * @param freq Noise frequency (0-31)
     */
    void setNoiseFrequency(uint8_t freq);
    
    /**
     * @brief Set envelope frequency (period)
     * @param freq 16-bit envelope period
     */
    void setEnvelopeFrequency(uint16_t freq);
    
    /**
     * @brief Set envelope shape
     * @param cont Continue flag
     * @param att Attack flag
     * @param alt Alternate flag
     * @param hold Hold flag
     */
    void setEnvelopeShape(bool cont, bool att, bool alt, bool hold);
    
    /**
     * @brief Reset the YM2149 to default state
     */
    void reset();
    
private:
    uint16_t _baseAddr;
    uint8_t _mixer;
};

#endif // YM2149_H
