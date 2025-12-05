# Papilio Audio Library Integration Guide

This document describes how to integrate the papilio_audio library into your Papilio Arcade project.

## Overview

The papilio_audio library provides Arduino-compatible APIs for controlling classic retro sound chips:
- **SID 6581** - Commodore 64 sound chip
- **YM2149** - AY-3-8910 compatible PSG (Atari ST, ZX Spectrum 128)
- **POKEY** - Atari 8-bit and arcade sound chip

## Wishbone Address Map

| Component | Base Address | Size | Description |
|-----------|--------------|------|-------------|
| SID 6581  | 0x8200       | 32 bytes | Commodore 64 SID chip |
| YM2149    | 0x8220       | 32 bytes | AY-3-8910 PSG |
| POKEY     | 0x8240       | 32 bytes | Atari POKEY |
| Audio Mixer | 0x8260     | 32 bytes | Mixer control |

## Firmware Integration

### 1. Add to platformio.ini

```ini
lib_deps = 
    https://github.com/GadgetFactory/papilio_wishbone_spi_master.git
    https://github.com/GadgetFactory/papilio_audio.git
```

### 2. Include in your sketch

```cpp
#include <SPI.h>
#include <PapilioAudio.h>

// SPI pins for ESP32-S3
#define SPI_SCK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// Create sound chip instances
SID6581 sid(WB_AUDIO_SID_BASE);
YM2149 ym(WB_AUDIO_YM2149_BASE);
POKEY pokey(WB_AUDIO_POKEY_BASE);

void setup() {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    wishboneInit(&SPI, SPI_CS);
    
    sid.begin();
    ym.begin();
    pokey.begin();
}
```

## SID6581 API

### Voice Methods (V1, V2, V3)
- `setNote(note, active)` - Set MIDI note and gate
- `setFreq(freq)` - Set frequency directly
- `setTriangle(active)` - Enable triangle waveform
- `setSawtooth(active)` - Enable sawtooth waveform
- `setSquare(active, pwm)` - Enable square wave with PWM
- `setNoise(active)` - Enable noise
- `setEnvelopeAttack(rate)` - Set attack rate (0-15)
- `setEnvelopeDecay(rate)` - Set decay rate (0-15)
- `setEnvelopeSustain(level)` - Set sustain level (0-15)
- `setEnvelopeRelease(rate)` - Set release rate (0-15)
- `setGate(active)` - Control note gate
- `reset()` - Reset voice to defaults

### Global Methods
- `setVolume(volume)` - Set master volume (0-15)
- `setFilterCutoff(freq)` - Set filter cutoff frequency
- `setFilterResonance(res)` - Set filter resonance (0-15)
- `setFilterMode(lp, bp, hp)` - Set filter mode
- `reset()` - Reset all voices

## SIDPlayer API

### Methods
- `begin()` - Initialize the player
- `loadFromMemory(data, length, subSong)` - Load .sid from memory
- `loadFile(filename, subSong)` - Load .sid from LittleFS
- `play(active)` - Start/stop playback
- `isPlaying()` - Check if playing
- `timerCallback()` - Call at 50Hz from timer
- `update()` - Call from main loop
- `getTitle()` - Get song title
- `getAuthor()` - Get song author
- `getCopyright()` - Get copyright
- `getNumSongs()` - Get number of sub-songs
- `nextSong()` - Play next sub-song
- `prevSong()` - Play previous sub-song

## Clock Requirements

| Chip | Clock | Notes |
|------|-------|-------|
| SID 6581 | 1 MHz | PAL: 985248 Hz, NTSC: 1022727 Hz |
| YM2149 | 2 MHz | Derived from system clock |
| POKEY | 1.79 MHz | NTSC timing |

## Audio Output

Connect the sigma-delta DAC output through an RC low-pass filter (10kΩ + 100nF) for analog audio.

## License

GPL-3.0
