# Papilio Audio Library

Retro sound chip emulation for the Papilio Arcade FPGA board.

## Supported Sound Chips

### SID 6581 (Commodore 64)
The legendary SID chip from the Commodore 64, featuring:
- 3 independent voices
- ADSR envelope generator per voice
- Multiple waveforms (triangle, sawtooth, pulse, noise)
- Ring modulation and sync
- Multi-mode filter (low-pass, band-pass, high-pass)

### YM2149 (AY-3-8910 compatible)
Popular sound chip used in Atari ST, ZX Spectrum 128, and many arcade games:
- 3 square wave tone generators
- 1 noise generator
- Volume envelope generator
- Mixer control

### POKEY (Atari 8-bit)
The POKEY chip from Atari computers and arcade games:
- 4 audio channels
- Flexible frequency divider options
- Noise generator with multiple polynomials
- High-pass filter

## Quick Start

```cpp
#include <SPI.h>
#include <PapilioAudio.h>

SID6581 sid(WB_AUDIO_SID_BASE);

void setup() {
    SPI.begin(12, 13, 11, 10);  // SCK, MISO, MOSI, CS
    wishboneInit(&SPI, 10);
    
    sid.begin();
    sid.setVolume(15);
    
    // Configure voice 1
    sid.V1.setTriangle(true);
    sid.V1.setEnvelopeAttack(0);
    sid.V1.setEnvelopeDecay(0);
    sid.V1.setEnvelopeSustain(15);
    sid.V1.setEnvelopeRelease(0);
    
    // Play a note
    sid.V1.setNote(60, true);  // Middle C
}

void loop() {
    // Your audio code here
}
```

## Wishbone Address Map

| Sound Chip | Base Address | Size |
|------------|--------------|------|
| SID 6581   | 0x8200       | 32 bytes |
| YM2149     | 0x8220       | 32 bytes |
| POKEY      | 0x8240       | 32 bytes |
| Audio Mixer| 0x8260       | 32 bytes |

## SID File Player

The library includes a complete .sid file player with 6502 CPU emulation:

```cpp
#include <PapilioAudio.h>
#include <LittleFS.h>
#include <Ticker.h>

SID6581 sid(WB_AUDIO_SID_BASE);
SIDPlayer player(&sid);
Ticker playTimer;

void onTimer() { player.timerCallback(); }

void setup() {
    LittleFS.begin(true);
    SPI.begin(12, 13, 11, 10);
    wishboneInit(&SPI, 10);
    
    player.begin();
    player.loadFile("/music.sid");
    playTimer.attach_ms(20, onTimer);  // 50Hz for PAL
    player.play(true);
}

void loop() {
    player.update();
}
```

## API Reference

See [INTEGRATION.md](INTEGRATION.md) for detailed API documentation.

## Hardware Requirements

- Papilio Arcade board (Tang Nano 20K with Gowin GW2A-18C FPGA)
- ESP32-S3 microcontroller
- Audio output connected to FPGA pin (via RC filter)

## License

GPL-3.0

Original sound chip implementations based on work from:
- SID: Alvaro Lopes (alvieboy@alvie.com)
- YM2149: MikeJ (fpgaarcade.com)
- POKEY: MikeJ (fpgaarcade.com)
