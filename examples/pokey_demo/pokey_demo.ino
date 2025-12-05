/**
 * @file pokey_demo.ino
 * @brief POKEY (Atari) sound chip demo
 * 
 * Demonstrates the distinctive sounds of the POKEY chip
 * used in Atari computers and arcade games.
 * 
 * NOTE: This requires the POKEY gateware to be loaded into the FPGA.
 */

#include <SPI.h>
#include <PapilioAudio.h>

// SPI pins for ESP32-S3
#define SPI_SCK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// Create POKEY instance with default base address
POKEY pokey(WB_AUDIO_POKEY_BASE);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("POKEY Demo");
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    
    // Initialize Wishbone SPI interface
    wishboneInit(&SPI, SPI_CS);
    
    // Initialize POKEY
    pokey.begin();
    
    Serial.println("POKEY initialized");
}

void loop() {
    // Demo 1: Pure tone sweep (Asteroids-style)
    Serial.println("Pure tone sweep (Asteroids style)...");
    pokey.CH1.setDistortion(POKEY_DIST_PURE_TONE);
    pokey.CH1.setVolume(15);
    
    for (int freq = 255; freq >= 20; freq -= 5) {
        pokey.CH1.setFrequency(freq);
        delay(30);
    }
    pokey.CH1.setVolume(0);
    delay(500);
    
    // Demo 2: Laser shot effect
    Serial.println("Laser shot effect...");
    pokey.CH2.setDistortion(POKEY_DIST_POLY4_ONLY);
    for (int shot = 0; shot < 3; shot++) {
        for (int freq = 10; freq < 200; freq += 10) {
            pokey.CH2.setFrequency(freq);
            pokey.CH2.setVolume(15 - (freq / 15));
            delay(10);
        }
        pokey.CH2.setVolume(0);
        delay(200);
    }
    delay(500);
    
    // Demo 3: Engine rumble (Battlezone-style)
    Serial.println("Engine rumble (Battlezone style)...");
    pokey.setAUDCTL(POKEY_AUDCTL_POLY9);  // Use 9-bit poly
    pokey.CH3.setDistortion(POKEY_DIST_POLY17_ONLY);
    pokey.CH3.setFrequency(200);
    
    for (int i = 0; i < 50; i++) {
        pokey.CH3.setVolume(8 + (i % 8));
        delay(50);
    }
    pokey.CH3.setVolume(0);
    pokey.setAUDCTL(0);
    delay(500);
    
    // Demo 4: Explosion effect
    Serial.println("Explosion effect...");
    pokey.CH4.setDistortion(POKEY_DIST_POLY5_POLY17);
    pokey.CH4.setFrequency(50);
    
    for (int vol = 15; vol >= 0; vol--) {
        pokey.CH4.setVolume(vol);
        pokey.CH4.setFrequency(50 + (15 - vol) * 10);
        delay(80);
    }
    delay(500);
    
    // Demo 5: Musical sequence (simple melody)
    Serial.println("Musical sequence...");
    pokey.CH1.setDistortion(POKEY_DIST_PURE_TONE);
    
    // Simple melody using raw frequency values
    const uint8_t melody[] = {60, 80, 60, 80, 50, 50, 60, 80, 100, 120};
    const int melodyLen = sizeof(melody);
    
    for (int note = 0; note < melodyLen; note++) {
        pokey.CH1.setFrequency(melody[note]);
        pokey.CH1.setVolume(12);
        delay(150);
        pokey.CH1.setVolume(0);
        delay(50);
    }
    delay(500);
    
    // Demo 6: Dual-channel harmony
    Serial.println("Two-channel harmony...");
    pokey.CH1.setDistortion(POKEY_DIST_PURE_TONE);
    pokey.CH2.setDistortion(POKEY_DIST_PURE_TONE);
    
    pokey.CH1.setFrequency(60);
    pokey.CH2.setFrequency(75);  // Roughly a fifth above
    pokey.CH1.setVolume(10);
    pokey.CH2.setVolume(10);
    
    delay(500);
    
    // Detune effect
    for (int i = 0; i < 20; i++) {
        pokey.CH2.setFrequency(75 + (i % 5));
        delay(50);
    }
    
    pokey.CH1.setVolume(0);
    pokey.CH2.setVolume(0);
    
    Serial.println("Restarting demo...");
    delay(2000);
}
