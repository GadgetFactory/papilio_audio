/**
 * @file SID6581.h
 * @brief SID 6581 (Commodore 64) sound chip emulation API
 * 
 * The SID (Sound Interface Device) was the audio chip used in the 
 * Commodore 64 and is famous for its distinctive sound.
 * 
 * Features:
 * - 3 independent voices
 * - ADSR envelope generator per voice
 * - Multiple waveforms: triangle, sawtooth, pulse, noise
 * - Ring modulation and oscillator sync
 * - Multi-mode resonant filter
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#ifndef SID6581_H
#define SID6581_H

#include <Arduino.h>
#include "WishboneSPI.h"

// SID Register offsets (relative to voice base)
#define SID_VOICE_FREQ_LO       0x00
#define SID_VOICE_FREQ_HI       0x01
#define SID_VOICE_PW_LO         0x02
#define SID_VOICE_PW_HI         0x03
#define SID_VOICE_CONTROL       0x04
#define SID_VOICE_ATTACK_DECAY  0x05
#define SID_VOICE_SUSTAIN_RELEASE 0x06

// Voice base addresses (offset from SID base)
#define SID_VOICE1_BASE         0x00
#define SID_VOICE2_BASE         0x07
#define SID_VOICE3_BASE         0x0E

// Global SID registers (offset from SID base)
#define SID_FILTER_FC_LO        0x15
#define SID_FILTER_FC_HI        0x16
#define SID_FILTER_RES_FILT     0x17
#define SID_FILTER_MODE_VOL     0x18

// SID Control register bits
#define SID_CTRL_GATE           0x01
#define SID_CTRL_SYNC           0x02
#define SID_CTRL_RING_MOD       0x04
#define SID_CTRL_TEST           0x08
#define SID_CTRL_TRIANGLE       0x10
#define SID_CTRL_SAWTOOTH       0x20
#define SID_CTRL_SQUARE         0x40
#define SID_CTRL_NOISE          0x80

/**
 * @class SIDVoice
 * @brief Represents a single SID voice with its associated registers
 */
class SIDVoice {
public:
    SIDVoice();
    
    /**
     * @brief Initialize the voice with base address
     * @param baseAddr Base address of this voice (SID base + voice offset)
     */
    void begin(uint16_t baseAddr);
    
    /**
     * @brief Set frequency from MIDI note number
     * @param note MIDI note number (0-127)
     * @param active If true, also sets the gate
     */
    void setNote(uint8_t note, bool active);
    
    /**
     * @brief Set raw frequency value
     * @param freq 16-bit frequency value
     */
    void setFreq(uint16_t freq);
    
    /**
     * @brief Get current frequency
     * @return Current frequency value
     */
    uint16_t getCurrentFreq();
    
    /**
     * @brief Set pulse width (duty cycle)
     * @param pw 12-bit pulse width value (0-4095)
     */
    void setPulseWidth(uint16_t pw);
    
    /**
     * @brief Set low byte of pulse width
     * @param pw Low 8 bits of pulse width
     */
    void setPWLo(uint8_t pw);
    
    /**
     * @brief Set high nybble of pulse width
     * @param pw High 4 bits of pulse width
     */
    void setPWHi(uint8_t pw);
    
    /**
     * @brief Control the gate (note on/off)
     * @param active True to turn note on
     */
    void setGate(bool active);
    
    /**
     * @brief Enable oscillator sync
     * @param active True to enable
     */
    void setSync(bool active);
    
    /**
     * @brief Enable ring modulation
     * @param active True to enable
     */
    void setRingMod(bool active);
    
    /**
     * @brief Enable test bit (resets oscillator)
     * @param active True to enable
     */
    void setTest(bool active);
    
    /**
     * @brief Enable triangle waveform
     * @param active True to enable
     */
    void setTriangle(bool active);
    
    /**
     * @brief Enable sawtooth waveform
     * @param active True to enable
     */
    void setSawtooth(bool active);
    
    /**
     * @brief Enable square wave
     * @param active True to enable
     * @param pwm Optional pulse width modulation value
     */
    void setSquare(bool active, uint16_t pwm = 2048);
    
    /**
     * @brief Enable noise waveform
     * @param active True to enable
     */
    void setNoise(bool active);
    
    /**
     * @brief Set envelope attack rate
     * @param rate Attack rate (0-15)
     */
    void setEnvelopeAttack(uint8_t rate);
    
    /**
     * @brief Set envelope decay rate
     * @param rate Decay rate (0-15)
     */
    void setEnvelopeDecay(uint8_t rate);
    
    /**
     * @brief Set envelope sustain level
     * @param level Sustain level (0-15)
     */
    void setEnvelopeSustain(uint8_t level);
    
    /**
     * @brief Set envelope release rate
     * @param rate Release rate (0-15)
     */
    void setEnvelopeRelease(uint8_t rate);
    
    /**
     * @brief Configure a complete instrument preset
     */
    void setInstrument(const char* name, uint8_t attack, uint8_t decay, 
                       uint8_t sustain, uint8_t release,
                       bool noise, bool square, bool sawtooth, bool triangle,
                       uint16_t pwm = 2048);
    
    /**
     * @brief Reset this voice to default state
     */
    void reset();
    
private:
    uint16_t _baseAddr;
    uint16_t _currentFreq;
    uint8_t _controlReg;
    uint8_t _attackDecay;
    uint8_t _sustainRelease;
    
    void writeReg(uint8_t offset, uint8_t value);
    void updateControlReg();
    void updateADSR();
    
    // MIDI to SID frequency conversion table
    static const uint16_t MIDI2freq[129];
};

/**
 * @class SID6581
 * @brief Complete SID chip with 3 voices and global controls
 */
class SID6581 {
public:
    SIDVoice V1;  ///< Voice 1
    SIDVoice V2;  ///< Voice 2
    SIDVoice V3;  ///< Voice 3
    
    /**
     * @brief Constructor
     * @param baseAddr Base address for SID chip (default 0x30)
     */
    SID6581(uint16_t baseAddr = 0x30);
    
    /**
     * @brief Initialize the SID chip
     */
    void begin();
    
    /**
     * @brief Write directly to a SID register
     * @param addr Register offset
     * @param data Data to write
     */
    void writeReg(uint8_t addr, uint8_t data);
    
    /**
     * @brief Read from a SID register
     * @param addr Register offset
     * @return Register value
     */
    uint8_t readReg(uint8_t addr);
    
    /**
     * @brief Set master volume
     * @param volume Volume level (0-15)
     */
    void setVolume(uint8_t volume);
    
    /**
     * @brief Set filter cutoff frequency
     * @param freq 11-bit cutoff frequency
     */
    void setFilterCutoff(uint16_t freq);
    
    /**
     * @brief Set filter resonance
     * @param resonance Resonance level (0-15)
     */
    void setFilterResonance(uint8_t resonance);
    
    /**
     * @brief Enable filter for specific voice
     * @param voice Voice number (1-3)
     * @param enable True to enable filter
     */
    void setFilterEnable(uint8_t voice, bool enable);
    
    /**
     * @brief Set filter mode
     * @param lowpass Enable low-pass filter
     * @param bandpass Enable band-pass filter
     * @param highpass Enable high-pass filter
     */
    void setFilterMode(bool lowpass, bool bandpass, bool highpass);
    
    /**
     * @brief Reset the entire SID chip
     */
    void reset();
    
private:
    uint16_t _baseAddr;
    uint8_t _modeVolume;
    uint8_t _resFilt;
};

#endif // SID6581_H
