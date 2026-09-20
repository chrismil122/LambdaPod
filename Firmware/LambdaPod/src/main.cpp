#include "AudioTools.h"
//#include "AudioTools/Communication/A2DPStream.h"
//#include "BluetoothA2DPSource.h"
//#include "ASP.h"
#include "Display.h"
#include "SDSPI.h"
#include <SD.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
//#include "BtScanner.h"

// Define your VSPI pins for the SD card
//#define VSPI_SCLK 18
//#define VSPI_MISO 19
//#define VSPI_MOSI 23
//#define VSPI_SS   5

//SDSpi sd;
// AudioTools Global Objects
//A2DPStream a2dpSource;
// Assuming Asp is instantiated globally, passing the source and your custom SD handler
//Asp asp(&a2dpSource, sd);
// Display Object
Display display;
TFT_eSPI tft = TFT_eSPI();
SDSpi sd;
MetaDataID3 id3Parser; 
//BtScanner Object
//BtScanner bt;

// -------------------------------------------------------------
// HARDWARE PIN DEFINITIONS
// -------------------------------------------------------------
#define SD_CS 5            // Pin connected to the SD card Chip Select (Adjust for your board)
#define TFT_CS 22

int16_t screenX = ((tft.width()/2)-(135/2));
int16_t screenY = (tft.height()-135);

void setup() {
    //Serial.println("[System] Mounting SD storage...");
    //SPI.begin(VSPI_SCLK, VSPI_MISO, VSPI_MOSI, 5);
    //sd.Init(); // Call your SD constructor/init abstraction here
    Serial.begin(115200);
    display.Init();
    tft.drawString("System Booting...", 10, 10, 2);
    if (sd.Init()){
        tft.drawString("SD Storage Ready.", 10, 30, 2);
    } else {
        tft.drawString("SD Storage Failed.", 10, 30, 2);
        while(true);
    }
    delay(3000);
    display.Draw(TFT_BLACK);

    // Run the full execution cycle
    display.processFastThumbnail("/formalin.mp3", screenX, screenY);
    delay(500);
    display.displayTrackInfo("/formalin.mp3", screenX, screenY);
    //if (SD.begin(5)) {
    //    display.Draw(TFT_GREEN);
    //} else {
    //    display.Draw(TFT_RED);
    //}
    
    //Serial.println("[System] Allocating Bluedroid Engine Core...");
    //auto a2dpConfig = a2dpSource.defaultConfig(TX_MODE);
    //a2dpSource.begin(a2dpConfig); 

    //Serial.println("\n[Boot] Listenwing for headset autoconnect flags... (10s)");
    //bt.stateTimer = millis();
}
void loop(){}

/*
void loop() {
    // Keep feeding data to the headphones if playing
    if (bt.currentState == STATE_PLAYING) {
        asp.Tick();
        
        // Loop back to beginning if song finishes naturally
        if (!asp.IsPlaying()) { 
            Serial.println("[Audio] Track finished. Restarting loop...");
            asp.PlayFile("/zhizn.mp3");
        }
    }

    // Handle serial inputs for manual interventions anywhere
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == "scan") {
            if (bt.currentState == STATE_PLAYING) {
                Serial.println("[System] Halting active playback for scanning routines...");
                // Stop audio streaming safely
                //asp.Stop(); 
            }
            // Force break any active link to look for new hardware
            if (a2dpSource.source().is_connected()) {
                Serial.println("[Linker] Severing current link-layer channel...");
                a2dpSource.source().disconnect();
                delay(500);
            }
            bt.startScanning();
        } 
        else if (bt.currentState == STATE_SCANNING && command.startsWith("connect ")) {
            int index = command.substring(8).toInt();
            bt.connectToDevice(index);
        }
    }

    // State Machine Evaluators
    switch (bt.currentState) {
        case STATE_AUTOCONNECT_WAIT:
            if (a2dpSource.source().is_connected()) {
                Serial.println("\n[Linker] Verified active autoconnect!");
                bt.currentState = STATE_PLAYING;
                asp.PlayFile("/zhizn.mp3");
            } else if (millis() - bt.stateTimer > bt.autoconnectTimeout) {
                Serial.println("\n[Boot] Autoconnect window expired with no link. Standing by.");
                Serial.println("Type 'scan' into console to discover available audio devices.");
                bt.currentState = STATE_IDLE;
            }
            break;

        case STATE_SCANNING:
            // Check if the low level link toggled active during scanning or connection sequences
            if (a2dpSource.source().is_connected()) {
                Serial.println("\n[Linker] Core connection acknowledged successfully!");
                bt.currentState = STATE_PLAYING;
                asp.PlayFile("/zhizn.mp3");
            }
            break;

        case STATE_PLAYING:
            // Track physical dropouts dynamically
            if (!a2dpSource.source().is_connected()) {
                Serial.println("\n[Warning] Link severed or headphones powered off. Dropping to Idle.");
                bt.currentState = STATE_IDLE;
            }
            break;

        case STATE_IDLE:
            // Watch for sudden external connections even when sitting idle
            if (a2dpSource.source().is_connected()) {
                Serial.println("\n[Linker] Re-established connection automatically!");
                bt.currentState = STATE_PLAYING;
                asp.PlayFile("/zhizn.mp3");
            }
            break;
    }
}
*/