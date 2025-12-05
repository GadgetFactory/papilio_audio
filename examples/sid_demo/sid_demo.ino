/**
 * @file sid_demo.ino
 * @brief SID 6581 (Commodore 64) sound chip demo
 * 
 * Demonstrates playing notes and using different waveforms
 * on the SID sound chip emulated in the FPGA.
 * 
 * NOTE: This requires the SID gateware to be loaded into the FPGA.
 */

#include <SPI.h>
#include <PapilioAudio.h>

// SPI pins for ESP32-S3
#define SPI_SCK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// Create SID instance with default base address
SID6581 sid(WB_AUDIO_SID_BASE);

// Scale notes (C major scale)
const uint8_t scale[] = {60, 62, 64, 65, 67, 69, 71, 72};
const int NUM_NOTES = sizeof(scale);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("SID 6581 Demo");
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    
    // Initialize Wishbone SPI interface
    wishboneInit(&SPI, SPI_CS);
    
    // Initialize SID
    sid.begin();
    
    // Set master volume
    sid.setVolume(15);
    
    // Configure Voice 1 - Triangle wave (soft, flute-like)
    sid.V1.setTriangle(true);
    sid.V1.setEnvelopeAttack(2);
    sid.V1.setEnvelopeDecay(4);
    sid.V1.setEnvelopeSustain(10);
    sid.V1.setEnvelopeRelease(6);
    
    // Configure Voice 2 - Sawtooth wave (bright, string-like)
    sid.V2.setSawtooth(true);
    sid.V2.setEnvelopeAttack(0);
    sid.V2.setEnvelopeDecay(8);
    sid.V2.setEnvelopeSustain(4);
    sid.V2.setEnvelopeRelease(8);
    
    // Configure Voice 3 - Square wave with PWM (classic synth)
    sid.V3.setSquare(true, 2048);
    sid.V3.setEnvelopeAttack(4);
    sid.V3.setEnvelopeDecay(6);
    sid.V3.setEnvelopeSustain(8);
    sid.V3.setEnvelopeRelease(4);
    
    Serial.println("Playing C major scale on 3 voices...");
}

void loop() {
    // Play ascending scale on Voice 1
    Serial.println("Voice 1 - Triangle wave");
    for (int i = 0; i < NUM_NOTES; i++) {
        sid.V1.setNote(scale[i], true);
        delay(300);
        sid.V1.setGate(false);
        delay(50);
    }
    delay(500);
    
    // Play descending scale on Voice 2
    Serial.println("Voice 2 - Sawtooth wave");
    for (int i = NUM_NOTES - 1; i >= 0; i--) {
        sid.V2.setNote(scale[i], true);
        delay(300);
        sid.V2.setGate(false);
        delay(50);
    }
    delay(500);
    
    // Play chord on Voice 3
    Serial.println("Voice 3 - Square wave chord");
    sid.V1.setNote(60, true);  // C
    sid.V2.setNote(64, true);  // E
    sid.V3.setNote(67, true);  // G
    delay(1000);
    
    // Release all
    sid.V1.setGate(false);
    sid.V2.setGate(false);
    sid.V3.setGate(false);
    delay(1000);
    
    // Demo filter sweep
    Serial.println("Filter sweep demo...");
    sid.V1.setNote(60, true);
    sid.setFilterEnable(1, true);
    sid.setFilterMode(true, false, false);  // Low-pass
    sid.setFilterResonance(8);
    
    for (uint16_t fc = 0; fc < 2047; fc += 32) {
        sid.setFilterCutoff(fc);
        delay(20);
    }
    
    sid.V1.setGate(false);
    sid.setFilterEnable(1, false);
    delay(1000);
    
    Serial.println("Restarting demo...");
    delay(2000);
}
