/**
 * @file ym2149_demo.ino
 * @brief YM2149 (AY-3-8910) sound chip demo
 * 
 * Demonstrates playing notes and using the envelope generator
 * on the YM2149 PSG chip emulated in the FPGA.
 * 
 * NOTE: This requires the YM2149 gateware to be loaded into the FPGA.
 */

#include <SPI.h>
#include <PapilioAudio.h>

// SPI pins for ESP32-S3
#define SPI_SCK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// Create YM2149 instance with default base address
YM2149 ym(WB_AUDIO_YM2149_BASE);

// Arpeggio notes
const uint8_t arpeggio[] = {60, 64, 67, 72, 67, 64};
const int NUM_NOTES = sizeof(arpeggio);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("YM2149 Demo");
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    
    // Initialize Wishbone SPI interface
    wishboneInit(&SPI, SPI_CS);
    
    // Initialize YM2149
    ym.begin();
    
    // Enable all three voices with tone
    ym.V1.setVolume(15);
    ym.V1.setTone(true);
    
    ym.V2.setVolume(15);
    ym.V2.setTone(true);
    
    ym.V3.setVolume(15);
    ym.V3.setTone(true);
    
    Serial.println("YM2149 initialized");
}

void loop() {
    // Demo 1: Simple arpeggio on Voice 1
    Serial.println("Playing arpeggio on Voice 1...");
    for (int repeat = 0; repeat < 3; repeat++) {
        for (int i = 0; i < NUM_NOTES; i++) {
            ym.V1.setNote(arpeggio[i], true);
            delay(150);
        }
    }
    delay(500);
    
    // Demo 2: Three-voice chord
    Serial.println("Playing chord on all voices...");
    ym.V1.setNote(60, true);  // C
    ym.V2.setNote(64, true);  // E
    ym.V3.setNote(67, true);  // G
    delay(1000);
    
    // Demo 3: Noise effect
    Serial.println("Adding noise...");
    ym.setNoiseFrequency(15);
    ym.V1.setNoise(true);
    ym.V1.setVolume(8);
    delay(500);
    ym.V1.setNoise(false);
    ym.V1.setVolume(15);
    
    // Demo 4: Envelope sweep
    Serial.println("Envelope demo...");
    
    // Use envelope for all voices
    ym.V1.setEnvelope(true);
    ym.V2.setEnvelope(true);
    ym.V3.setEnvelope(true);
    
    // Set notes
    ym.V1.setNote(48, true);  // C (lower)
    ym.V2.setNote(52, true);  // E
    ym.V3.setNote(55, true);  // G
    
    // Sawtooth envelope (attack)
    ym.setEnvelopeFrequency(2000);
    ym.setEnvelopeShape(true, true, false, false);  // Attack, repeat
    delay(2000);
    
    // Triangle envelope
    ym.setEnvelopeShape(true, true, true, false);  // Attack, alternate
    delay(2000);
    
    // Return to fixed volume
    ym.V1.setEnvelope(false);
    ym.V2.setEnvelope(false);
    ym.V3.setEnvelope(false);
    ym.V1.setVolume(15);
    ym.V2.setVolume(15);
    ym.V3.setVolume(15);
    
    // Demo 5: Volume fade out
    Serial.println("Fade out...");
    for (int vol = 15; vol >= 0; vol--) {
        ym.V1.setVolume(vol);
        ym.V2.setVolume(vol);
        ym.V3.setVolume(vol);
        delay(100);
    }
    
    delay(500);
    
    // Silence
    ym.V1.setTone(false);
    ym.V2.setTone(false);
    ym.V3.setTone(false);
    
    Serial.println("Restarting demo...");
    delay(2000);
    
    // Re-enable tones
    ym.V1.setTone(true);
    ym.V1.setVolume(15);
    ym.V2.setTone(true);
    ym.V2.setVolume(15);
    ym.V3.setTone(true);
    ym.V3.setVolume(15);
}
