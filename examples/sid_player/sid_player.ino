/**
 * @file sid_player.ino
 * @brief SID Player demo - plays Commodore 64 music files
 * 
 * Demonstrates playing .sid music files using a 6502 CPU emulator
 * and the SID 6581 sound chip emulated in the FPGA.
 * 
 * NOTE: This requires the SID gateware to be loaded into the FPGA.
 * 
 * Hardware:
 *   - Papilio Arcade board with SID gateware
 *   - Audio output on audio_left/audio_right pins
 */

#include <SPI.h>
#include <PapilioAudio.h>
#include <Ticker.h>

// SPI pins for ESP32-S3
#define SPI_SCK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// Embedded SID tune - "Layla Mix" by Edwin van Santen
// From 20th Century Composers (1988)
#include <tunes/sidtune_Layla_Mix.h>

// Create SID and player instances
SID6581 sid(WB_AUDIO_SID_BASE);
SIDPlayer player(&sid);

// Timer for 50Hz playback interrupt
Ticker playTimer;

// Flag for timer callback
volatile bool timerFlag = false;

void IRAM_ATTR onTimer() {
    timerFlag = true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("  SID Player Demo");
    Serial.println("  Commodore 64 Music Emulation");
    Serial.println("=================================");
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    
    // Initialize Wishbone SPI interface
    wishboneInit(&SPI, SPI_CS);
    
    // Initialize player
    player.begin();
    
    // Load the embedded SID tune
    Serial.println("Loading SID file...");
    if (player.loadFromMemory(sidtune_Layla_Mix, sizeof(sidtune_Layla_Mix))) {
        Serial.println("SID file loaded successfully!");
        Serial.print("Title: ");
        Serial.println(player.getTitle());
        Serial.print("Author: ");
        Serial.println(player.getAuthor());
        Serial.print("Copyright: ");
        Serial.println(player.getCopyright());
        Serial.print("Sub-songs: ");
        Serial.println(player.getNumSongs());
        
        // Start 50Hz timer (PAL timing)
        playTimer.attach_ms(20, onTimer);  // 50Hz = 20ms period
        
        // Start playback
        player.play(true);
        Serial.println("Playing...");
    } else {
        Serial.println("Error: Failed to load SID file!");
    }
    
    Serial.println("");
    Serial.println("Commands:");
    Serial.println("  p - Play/Pause");
    Serial.println("  n - Next song");
    Serial.println("  b - Previous song");
    Serial.println("  r - Restart");
}

void loop() {
    // Process timer callback
    if (timerFlag) {
        timerFlag = false;
        player.timerCallback();
    }
    
    // Update player (runs 6502 emulator)
    player.update();
    
    // Handle serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 'p':
            case 'P':
                if (player.isPlaying()) {
                    player.play(false);
                    Serial.println("Paused");
                } else {
                    player.play(true);
                    Serial.println("Playing");
                }
                break;
                
            case 'n':
            case 'N':
                player.nextSong();
                Serial.print("Playing sub-song ");
                Serial.println(player.getCurrentSong() + 1);
                break;
                
            case 'b':
            case 'B':
                player.prevSong();
                Serial.print("Playing sub-song ");
                Serial.println(player.getCurrentSong() + 1);
                break;
                
            case 'r':
            case 'R':
                player.loadFromMemory(sidtune_Layla_Mix, sizeof(sidtune_Layla_Mix));
                player.play(true);
                Serial.println("Restarted");
                break;
        }
    }
}
