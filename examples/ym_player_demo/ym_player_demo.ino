/**
 * @file ym_player_demo.ino
 * @brief YM music file player demo
 * 
 * Plays music.ymd file from LittleFS filesystem.
 * Upload the music.ymd file to LittleFS using PlatformIO's data folder.
 * 
 * NOTE: This requires the YM2149 gateware to be loaded into the FPGA.
 */

#define PAPILIO_MCP_ENABLED
#include <SPI.h>
#include <LittleFS.h>
#include <PapilioAudio.h>
#include <PapilioMCP.h>

// SPI pins for ESP32-S3
#define SPI_SCK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// Create YM2149 and player instances
YM2149 ym(WB_AUDIO_YM2149_BASE);
YMPlayer player(ym);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("YM Player Demo");
    
    // Initialize MCP
    PapilioMCP.begin();
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    
    // Initialize Wishbone SPI interface
    wishboneInit(&SPI, SPI_CS);
    
    // Initialize YM2149
    ym.begin();
    
    // Initialize player and LittleFS
    if (!player.begin()) {
        Serial.println("Failed to initialize player!");
        return;
    }
    
    // Load and play music file
    if (player.loadFile("/music.ymd")) {
        Serial.println("Playing music.ymd...");
        player.setVolume(15);  // Maximum volume
        player.play();
        Serial.println("Player started");
    } else {
        Serial.println("Failed to load music.ymd");
        Serial.println("Make sure to upload data folder to LittleFS!");
    }
}

void loop() {
    // Process MCP commands
    PapilioMCP.update();
    
    // Update player at 50Hz (20ms)
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 20) {
        lastUpdate = millis();
        player.update();
    }
    
    // Show status every 5 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        lastStatus = millis();
        if (player.isPlaying()) {
            Serial.println("Playing...");
        } else {
            Serial.println("Stopped");
        }
    }
}
