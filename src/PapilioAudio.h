/**
 * @file PapilioAudio.h
 * @brief Main header for Papilio Audio Library
 * 
 * Provides API for controlling retro sound chips (SID, YM2149, POKEY)
 * implemented in FPGA via Wishbone-over-SPI interface.
 * 
 * @author GadgetFactory
 * @license GPL-3.0
 */

#ifndef PAPILIO_AUDIO_H
#define PAPILIO_AUDIO_H

#include <Arduino.h>
#include <SPI.h>
#include "WishboneSPI.h"

// Include individual sound chip classes
#include "SID6581.h"
#include "YM2149.h"
#include "POKEY.h"
#include "AudioMixer.h"
#include "SIDPlayer.h"

// Default Wishbone base addresses for audio peripherals
// These match the address map in top.v gateware
#ifndef WB_AUDIO_SID_BASE
#define WB_AUDIO_SID_BASE     0x8200   // SID chip: 0x8200-0x821F (32 bytes)
#endif

#ifndef WB_AUDIO_YM2149_BASE
#define WB_AUDIO_YM2149_BASE  0x8220   // YM2149: 0x8220-0x823F (32 bytes)
#endif

#ifndef WB_AUDIO_POKEY_BASE
#define WB_AUDIO_POKEY_BASE   0x8240   // POKEY: 0x8240-0x825F (32 bytes)
#endif

#ifndef WB_AUDIO_MIXER_BASE
#define WB_AUDIO_MIXER_BASE   0x8260   // Audio mixer: 0x8260-0x827F (32 bytes)
#endif

#endif // PAPILIO_AUDIO_H
