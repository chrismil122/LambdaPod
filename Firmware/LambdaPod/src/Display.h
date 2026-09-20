#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>
#include "AudioTools.h"
#define AUDIO_TOOLS_ID3_UNICODE 1
#include <TFT_eSPI.h>
#include "Segoe_UI15.h"
#include <TJpg_Decoder.h>
#include <SPI.h>
#include "SDSPI.h"

#define TFT_CS 22
#define DISPLAY_ROTATION 0

static String extractedTitle  = "";
static String extractedArtist = "";
static String extractedAlbum  = "";

class Display {
    public:
    void Init();
    void Draw(uint32_t color);
    static void processFastThumbnail(const char* filePath, int x, int y);
    void displayTrackInfo(const char* filePath, int x, int y);
    
    Display();
    private:

   
    static bool jpegCallbackWrapper(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

    static void metadataCallback(audio_tools::MetaDataType type, const char* str, int len) {
        String val = String(str).substring(0, len);
        val.trim();

        switch(type) {
            case audio_tools::Title:  extractedTitle = val;  break;
            case audio_tools::Artist: extractedArtist = val; break;
            case audio_tools::Album:  extractedAlbum = val;  break;
            default: break;
    }
}

    bool handleCanvasBuffer(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

    static Display* instance;
    bool memoryCanvasBufferCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
    String fastBlockExtract(const char* filePath);
    uint16_t thumbnailCanvas[135 * 135];

    uint8_t sectorBuffer[2048]; 
    bool artExtractionComplete = false;

    uint16_t imgWidth = 0;                 
    uint16_t imgHeight = 0;  
};

#endif