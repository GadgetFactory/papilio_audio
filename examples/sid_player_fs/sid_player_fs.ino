/**
 * @file sid_player_fs.ino
 * @brief SID Player demo - loads .sid files from LittleFS
 * 
 * Demonstrates loading and playing .sid music files from the ESP32's
 * flash filesystem using LittleFS.
 * 
 * To upload SID files to the filesystem:
 * 1. Place .sid files in the 'data' folder in your project root
 * 2. Run: pio run -t uploadfs -e sid_player_fs
 * 
 * NOTE: This requires the SID gateware to be loaded into the FPGA.
 * 
 * Hardware:
 *   - Papilio Arcade board with SID gateware
 *   - Audio output on audio_left/audio_right pins
 */

#include <SPI.h>
#include <LittleFS.h>
#include <PapilioAudio.h>
#include <Ticker.h>

// SPI pins for ESP32-S3
#define SPI_SCK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// Create SID and player instances
SID6581 sid(WB_AUDIO_SID_BASE);
SIDPlayer player(&sid);

// Timer for 50Hz playback interrupt
Ticker playTimer;

// Flag for timer callback
volatile bool timerFlag = false;

// List of SID files found
#define MAX_SID_FILES 20
String sidFiles[MAX_SID_FILES];
int numSidFiles = 0;
int currentFileIndex = 0;

void IRAM_ATTR onTimer() {
    timerFlag = true;
}

void listSidFiles() {
    numSidFiles = 0;
    File root = LittleFS.open("/");
    if (!root) {
        Serial.println("Failed to open root directory");
        return;
    }
    
    File file = root.openNextFile();
    while (file && numSidFiles < MAX_SID_FILES) {
        String name = file.name();
        if (name.endsWith(".sid") || name.endsWith(".SID")) {
            sidFiles[numSidFiles] = "/" + name;
            Serial.print("Found: ");
            Serial.println(sidFiles[numSidFiles]);
            numSidFiles++;
        }
        file = root.openNextFile();
    }
    
    Serial.print("Total SID files found: ");
    Serial.println(numSidFiles);
}

bool loadCurrentFile() {
    if (numSidFiles == 0) {
        Serial.println("No SID files found!");
        return false;
    }
    
    Serial.print("Loading: ");
    Serial.println(sidFiles[currentFileIndex]);
    
    if (!player.loadFile(sidFiles[currentFileIndex].c_str())) {
        Serial.println("Failed to load SID file!");
        return false;
    }
    
    Serial.println("SID file loaded successfully!");
    Serial.print("Title: ");
    Serial.println(player.getTitle());
    Serial.print("Author: ");
    Serial.println(player.getAuthor());
    Serial.print("Copyright: ");
    Serial.println(player.getCopyright());
    Serial.print("Sub-songs: ");
    Serial.println(player.getNumSongs());
    
    return true;
}

void nextFile() {
    player.play(false);
    currentFileIndex = (currentFileIndex + 1) % numSidFiles;
    if (loadCurrentFile()) {
        player.play(true);
    }
}

void prevFile() {
    player.play(false);
    currentFileIndex = (currentFileIndex - 1 + numSidFiles) % numSidFiles;
    if (loadCurrentFile()) {
        player.play(true);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("  SID Player - Filesystem Demo");
    Serial.println("  Commodore 64 Music Player");
    Serial.println("=================================");
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed!");
        Serial.println("Make sure to upload filesystem first:");
        Serial.println("  pio run -t uploadfs -e sid_player_fs");
        while(1) delay(100);
    }
    Serial.println("LittleFS mounted successfully");
    
    // Initialize SPI for FPGA communication
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    wishboneInit(&SPI, SPI_CS);
    
    // Initialize SID and player
    player.begin();
    
    // List all SID files
    listSidFiles();
    
    if (numSidFiles == 0) {
        Serial.println("No SID files found in filesystem!");
        Serial.println("Place .sid files in 'data' folder and run:");
        Serial.println("  pio run -t uploadfs -e sid_player_fs");
        while(1) delay(100);
    }
    
    // Load first file
    if (loadCurrentFile()) {
        // Start 50Hz timer for PAL playback
        playTimer.attach(0.02, onTimer);  // 50Hz = 20ms
        
        Serial.println("\nPlaying...");
        player.play(true);
    }
    
    Serial.println("\nCommands:");
    Serial.println("  p     - Play/Pause");
    Serial.println("  n     - Next sub-song");
    Serial.println("  b     - Previous sub-song");
    Serial.println("  N     - Next file");
    Serial.println("  B     - Previous file");
    Serial.println("  r     - Restart current song");
    Serial.println("  l     - List all files");
}

void loop() {
    // Handle serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'p':
            case 'P':
                player.play(!player.isPlaying());
                Serial.println(player.isPlaying() ? "Playing" : "Paused");
                break;
            case 'n':
                player.nextSong();
                Serial.print("Sub-song: ");
                Serial.print(player.getCurrentSong() + 1);
                Serial.print("/");
                Serial.println(player.getNumSongs());
                break;
            case 'b':
                player.prevSong();
                Serial.print("Sub-song: ");
                Serial.print(player.getCurrentSong() + 1);
                Serial.print("/");
                Serial.println(player.getNumSongs());
                break;
            case 'N':
                nextFile();
                Serial.print("File: ");
                Serial.print(currentFileIndex + 1);
                Serial.print("/");
                Serial.println(numSidFiles);
                break;
            case 'B':
                prevFile();
                Serial.print("File: ");
                Serial.print(currentFileIndex + 1);
                Serial.print("/");
                Serial.println(numSidFiles);
                break;
            case 'r':
            case 'R':
                loadCurrentFile();
                player.play(true);
                Serial.println("Restarted");
                break;
            case 'l':
            case 'L':
                Serial.println("\nSID Files:");
                for (int i = 0; i < numSidFiles; i++) {
                    Serial.print(i == currentFileIndex ? "> " : "  ");
                    Serial.print(i + 1);
                    Serial.print(". ");
                    Serial.println(sidFiles[i]);
                }
                break;
        }
    }
    
    // Process timer tick
    if (timerFlag) {
        player.timerCallback();
        timerFlag = false;
    }
    
    // Update player (runs 6502 emulator)
    player.update();
}
