/**
 * @file POKEY.h
 * @brief POKEY (Atari) sound chip emulation API
 * 
 * The POKEY was the sound chip used in Atari 8-bit computers
 * and many Atari arcade games. Known for its distinctive sound
 * used in games like Asteroids, Centipede, and Tempest.
 * 
 * Features:
 * - 4 audio channels
 * - 8-bit frequency dividers per channel
 * - Noise/tone selection per channel
 * - Volume control per channel
 * - Multiple clock divider options
 * - Polynomial counter noise generation
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#ifndef POKEY_H
#define POKEY_H

#include <Arduino.h>
#include "WishboneSPI.h"

// POKEY Register addresses
#define POKEY_REG_AUDF1     0x00    // Audio frequency 1
#define POKEY_REG_AUDC1     0x01    // Audio control 1
#define POKEY_REG_AUDF2     0x02    // Audio frequency 2
#define POKEY_REG_AUDC2     0x03    // Audio control 2
#define POKEY_REG_AUDF3     0x04    // Audio frequency 3
#define POKEY_REG_AUDC3     0x05    // Audio control 3
#define POKEY_REG_AUDF4     0x06    // Audio frequency 4
#define POKEY_REG_AUDC4     0x07    // Audio control 4
#define POKEY_REG_AUDCTL    0x08    // Audio control

// AUDCTL bits
#define POKEY_AUDCTL_POLY9      0x80    // Use 9-bit poly instead of 17-bit
#define POKEY_AUDCTL_CH1_HICLK  0x40    // Channel 1 high-pass filter clocked by ch 3
#define POKEY_AUDCTL_CH2_HICLK  0x20    // Channel 2 high-pass filter clocked by ch 4
#define POKEY_AUDCTL_CH34_JOIN  0x10    // Join channels 3+4 for 16-bit freq
#define POKEY_AUDCTL_CH12_JOIN  0x08    // Join channels 1+2 for 16-bit freq
#define POKEY_AUDCTL_CH3_179MHZ 0x04    // Channel 3 uses 1.79MHz clock
#define POKEY_AUDCTL_CH1_179MHZ 0x02    // Channel 1 uses 1.79MHz clock
#define POKEY_AUDCTL_15KHZ      0x01    // Use 15kHz clock instead of 64kHz

// AUDC distortion values
#define POKEY_DIST_POLY5_POLY17 0x00    // 5-bit poly clocked by 17-bit poly
#define POKEY_DIST_POLY5_ONLY   0x20    // 5-bit poly only
#define POKEY_DIST_POLY5_POLY4  0x40    // 5-bit poly clocked by 4-bit poly
#define POKEY_DIST_POLY5_PURE   0x60    // 5-bit poly -> pure tone
#define POKEY_DIST_POLY17_ONLY  0x80    // 17-bit poly only
#define POKEY_DIST_PURE_TONE    0xA0    // Pure tone (square wave)
#define POKEY_DIST_POLY4_ONLY   0xC0    // 4-bit poly only
#define POKEY_DIST_POLY4_PURE   0xE0    // 4-bit poly -> pure tone

/**
 * @class POKEYChannel
 * @brief Represents a single POKEY audio channel
 */
class POKEYChannel {
public:
    POKEYChannel();
    
    /**
     * @brief Initialize the channel
     * @param baseAddr POKEY base address
     * @param freqAddr Frequency register address
     * @param ctrlAddr Control register address
     */
    void begin(uint16_t baseAddr, uint8_t freqAddr, uint8_t ctrlAddr);
    
    /**
     * @brief Set channel frequency
     * @param freq 8-bit frequency divider (0-255)
     */
    void setFrequency(uint8_t freq);
    
    /**
     * @brief Get current frequency
     * @return Current frequency value
     */
    uint8_t getFrequency();
    
    /**
     * @brief Set channel volume
     * @param volume Volume level (0-15)
     */
    void setVolume(uint8_t volume);
    
    /**
     * @brief Get current volume
     * @return Current volume level
     */
    uint8_t getVolume();
    
    /**
     * @brief Set distortion type
     * @param dist Distortion type (use POKEY_DIST_* constants)
     */
    void setDistortion(uint8_t dist);
    
    /**
     * @brief Enable/disable volume only mode (no tone)
     * @param active True for volume only
     */
    void setVolumeOnly(bool active);
    
    /**
     * @brief Reset channel to default state
     */
    void reset();
    
private:
    uint16_t _baseAddr;
    uint8_t _freqAddr;
    uint8_t _ctrlAddr;
    uint8_t _freq;
    uint8_t _ctrl;  // Volume in lower 4 bits, distortion in upper 4 bits
    
    void writeReg(uint8_t addr, uint8_t value);
    void updateCtrl();
};

/**
 * @class POKEY
 * @brief Complete POKEY chip with 4 channels and global controls
 */
class POKEY {
public:
    POKEYChannel CH1;  ///< Channel 1
    POKEYChannel CH2;  ///< Channel 2
    POKEYChannel CH3;  ///< Channel 3
    POKEYChannel CH4;  ///< Channel 4
    
    /**
     * @brief Constructor
     * @param baseAddr Base address for POKEY (default 0x60)
     */
    POKEY(uint16_t baseAddr = 0x60);
    
    /**
     * @brief Initialize the POKEY
     */
    void begin();
    
    /**
     * @brief Write to a POKEY register
     * @param addr Register address
     * @param data Data to write
     */
    void writeReg(uint8_t addr, uint8_t data);
    
    /**
     * @brief Read from a POKEY register
     * @param addr Register address
     * @return Register value
     */
    uint8_t readReg(uint8_t addr);
    
    /**
     * @brief Set audio control register
     * @param value AUDCTL value (use POKEY_AUDCTL_* constants)
     */
    void setAUDCTL(uint8_t value);
    
    /**
     * @brief Get current AUDCTL value
     * @return Current AUDCTL register value
     */
    uint8_t getAUDCTL();
    
    /**
     * @brief Enable 9-bit polynomial (instead of 17-bit)
     * @param enable True for 9-bit, false for 17-bit
     */
    void setPoly9(bool enable);
    
    /**
     * @brief Enable 15kHz base clock (instead of 64kHz)
     * @param enable True for 15kHz, false for 64kHz
     */
    void set15kHz(bool enable);
    
    /**
     * @brief Join channels 1+2 for 16-bit frequency
     * @param enable True to join
     */
    void joinChannels12(bool enable);
    
    /**
     * @brief Join channels 3+4 for 16-bit frequency
     * @param enable True to join
     */
    void joinChannels34(bool enable);
    
    /**
     * @brief Reset the POKEY to default state
     */
    void reset();
    
private:
    uint16_t _baseAddr;
    uint8_t _audctl;
};

#endif // POKEY_H
