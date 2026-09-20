#include "Display.h"

extern TFT_eSPI tft;
extern Display display;
extern SDSpi sd;
extern MetaDataID3 id3Parser; 

Display* Display::instance = nullptr;

Display::Display() {
    instance = this;
}

void Display::Init(){
    tft.init();
    tft.begin();
    tft.setRotation(DISPLAY_ROTATION);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
}

void Display::Draw(uint32_t color){
    tft.fillScreen(color);
}              

// THE MEMORY MATRICES REDIRECT CALL-BACK
bool Display::memoryCanvasBufferCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  // Stop the decoder early the moment all 135 vertical rows are filled
  if (y >= 135) return false; 
  if (x >= 135) return true; 

  for (int16_t row = 0; row < h; row++) {
    int16_t canvasY = y + row;
    if (canvasY >= 135) break; 

    for (int16_t col = 0; col < w; col++) {
      int16_t canvasX = x + col;
      if (canvasX >= 135) break; 

      // Map indexing to the new 135x135 layout geometry bounds
      int32_t canvasIndex = (canvasY * 135) + canvasX;
      uint16_t rawPixel = bitmap[(row * w) + col];
      
      // Flip the bytes natively in RAM to ensure the colors are perfect
      thumbnailCanvas[canvasIndex] = __builtin_bswap16(rawPixel);
    }
  }
  return true; 
}

// High-speed block-based scanner to extract art data
// Change the return type in your header and cpp file to String!
String Display::fastBlockExtract(const char* filePath) {
  artExtractionComplete = false;
  SPIClass &sharedSPI = tft.getSPIinstance(); 

  // 1. Initialize SD Card Bus
  if (!SD.begin(SD_CS, sharedSPI, 16000000)) { 
    Serial.println("SD mount failed!");
    return "";
  }

  // 2. Ensure Thumb directory exists
  if (!SD.exists("/Thumb")){
    Serial.println("Thumb folder not found, creating it...");
    SD.mkdir("/Thumb");
  }

  // 3. Open the audio file to calculate the cache path
  File audioFile = SD.open(filePath, FILE_READ);
  if (!audioFile) {
    Serial.println("Unable to open source audio file.");
    return "";
  }
  
  // Extract and clean filename
  String filename = String(audioFile.name());
  if (filename.startsWith("/")) filename = filename.substring(1);
  
  // Strip .mp3 extension and swap it for .jpg
  if (filename.endsWith(".mp3") || filename.endsWith(".MP3")) {
    filename = filename.substring(0, filename.length() - 4);
  }
  String thumbPath = "/Thumb/" + filename + ".jpg";
  
  audioFile.close(); // Close it cleanly before running operations

  // 4. Bypassing check: If the .jpg already exists, return early!
  if (SD.exists(thumbPath)) {
  File checkFile = SD.open(thumbPath, FILE_READ);
  size_t fileSize = checkFile.size();
  checkFile.close();

  if (fileSize > 0) {
    Serial.println("Found valid cached art (" + String(fileSize) + " bytes): " + thumbPath + ", skipping scan!");
    artExtractionComplete = true;
    return thumbPath;
  } else {
    Serial.println("Found empty cache file! Deleting and forcing re-scan...");
    SD.remove(thumbPath); // Wipe out the corrupt 0-byte file
  }
}

  // 5. Cache missed: Open file again to run the raw sector scan
  audioFile = SD.open(filePath, FILE_READ);
  if (!audioFile) return "";

  Serial.println("Scanning sectors for 'APIC'...");
  
  while (audioFile.available() && !artExtractionComplete) {
    // Read a robust 2048-byte block to speed up the hardware read overhead
    size_t bytesRead = audioFile.read(sectorBuffer, 2048);
    
    for (size_t i = 0; i < bytesRead - 4; i++) {
      // Look for the ID3v2 Album Art frame signature tag
      if (sectorBuffer[i] == 'A' && sectorBuffer[i+1] == 'P' && sectorBuffer[i+2] == 'I' && sectorBuffer[i+3] == 'C') {
        Serial.println("APIC signature found! Finding JPEG boundary...");
        
        long apicPos = audioFile.position() - bytesRead + i;
        audioFile.seek(apicPos);
        
        while (audioFile.available()) {
          if (audioFile.read() == 0xFF) {
            if (audioFile.peek() == 0xD8) { // JPEG Magic Start bytes (FF D8)
              audioFile.read(); 
              
              Serial.println("JPEG located. Fast-streaming payload to cache file...");
              
              // Open our new .jpg cache target file for writing
              File thumbFile = SD.open(thumbPath, FILE_WRITE);
              if (!thumbFile) {
                audioFile.close();
                return "";
              }
              
              // Write the initial JPEG header indicators
              thumbFile.write(0xFF);
              thumbFile.write(0xD8);

              // Stream up to 1.5MB of data to ensure the entire image block is saved
              size_t bytesToStream = 1500000; 
              while (bytesToStream > 0 && audioFile.available()) {
                size_t chunk = (bytesToStream > 2048) ? 2048 : bytesToStream;
                size_t r = audioFile.read(sectorBuffer, chunk);
                thumbFile.write(sectorBuffer, r);
                bytesToStream -= r;
              }
              
              thumbFile.close();
              artExtractionComplete = true;
              break;
            }
          }
        }
        break; // Drop out of scanning loop once processing is complete
      }
    }
  }
  
  audioFile.close();
  return thumbPath;
}

bool Display::handleCanvasBuffer(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= 135) return false; 
  if (x >= 135) return true; 

  for (int16_t row = 0; row < h; row++) {
    int16_t canvasY = y + row;
    if (canvasY >= 135) break; 

    // --- 180 DEGREE VERTICAL FLIP ---
    int16_t targetY = 134 - canvasY; 

    for (int16_t col = 0; col < w; col++) {
      int16_t canvasX = x + col;
      if (canvasX >= 135) break; 

      // --- 180 DEGREE HORIZONTAL FLIP ---
      int16_t targetX = 134 - canvasX; 

      // Map indexing to the inverted destination bounds
      int32_t canvasIndex = (targetY * 135) + targetX;
      uint16_t rawPixel = bitmap[(row * w) + col];
      
      // Flip the bytes natively in RAM to ensure the colors are perfect
      thumbnailCanvas[canvasIndex] = __builtin_bswap16(rawPixel);
    }
  }
  return true; 
}

bool Display::jpegCallbackWrapper(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (instance != nullptr) {
        return instance->handleCanvasBuffer(x, y, w, h, bitmap);
    }
    return false;
}

void Display::processFastThumbnail(const char* filePath, int x, int y) {
  memset(instance->thumbnailCanvas, 0, sizeof(instance->thumbnailCanvas));

  // Get the string path, not an unstable file object
  String imgPath = instance->fastBlockExtract(filePath);
  if (imgPath == "") return;

  if (instance->artExtractionComplete && imgPath.length() > 0) {
    // Both header parser and decoder look at the exact same path!
    int checkHeader = TJpgDec.getFsJpgSize(&(instance->imgWidth), &(instance->imgHeight), imgPath.c_str(), SD);
    
    if (checkHeader == 0) {
      TJpgDec.setJpgScale(8); 
      TJpgDec.setCallback(jpegCallbackWrapper);
      TJpgDec.drawFsJpg(0, 0, imgPath.c_str(), SD);

      // Reset SPI bus for display
      tft.begin();
      tft.fillScreen(TFT_BLACK);
      tft.setRotation(2);
      tft.setSwapBytes(false); 

      tft.pushImage(x, y, 135, 135, instance->thumbnailCanvas);
      tft.setRotation(DISPLAY_ROTATION);
    }
  }
}

void metadataCallback(audio_tools::MetaDataType type, const char* str, int len) {
    // Convert the incoming character array to a clean string
    String val = String(str).substring(0, len);
    val.trim();

    // Catch the specific data types as they stream through
    switch(type) {
        case audio_tools::Title:  extractedTitle = val;  break;
        case audio_tools::Artist: extractedArtist = val; break;
        case audio_tools::Album:  extractedAlbum = val;  break;
        default: break;
    }
}

void Display::displayTrackInfo(const char* filePath, int x, int y) {
  digitalWrite(TFT_CS, HIGH); 
  
  SPIClass &sharedSPI = tft.getSPIinstance(); 

  // Don't call SD.begin if it's already active
  if (SD.cardSize() == 0) {
    if (!SD.begin(SD_CS, sharedSPI, 4000000)) { // Drop to 4MHz for stability
      Serial.println("SD mount failed for metadata!");
      return;
    }
  }

  File audioFile = SD.open(filePath, FILE_READ);
  if (!audioFile) {
    Serial.println("Failed to open file for metadata parsing.");
    return;
  }

  // Clear our global strings for the new track
  extractedTitle  = "";
  extractedArtist = "";
  extractedAlbum  = "";

  Serial.println("Parsing ID3 text tags using callbacks...");

  // Attach the callback handler to the audio-tools parser
  id3Parser.setCallback(metadataCallback);
  id3Parser.begin();

  // Feed the first 32KB of the MP3 file to the stream parser
  size_t bytesToScan = 32768; 
  while (audioFile.available() && bytesToScan > 0) {
    size_t chunk = (bytesToScan > 512) ? 512 : bytesToScan;
    size_t r = audioFile.read(sectorBuffer, chunk);
    if (r == 0) break;
    
    // As bytes go in, metadataCallback automatically catches title/artist/album!
    id3Parser.write(sectorBuffer, r);
    bytesToScan -= r;
  }
  
  id3Parser.end();
  audioFile.close();

  // CRITICAL: Give the SD card a tiny 10ms moment to settle the SPI bus 
  // after closing a massive MP3 file stream before asking it to read a 7MB font file!
  delay(10); 

  // Fallbacks if tags are missing
  if (extractedTitle.length() == 0) {
    extractedTitle = String(filePath); 
    int lastSlash = extractedTitle.lastIndexOf('/');
    if (lastSlash >= 0) extractedTitle = extractedTitle.substring(lastSlash + 1);
  }
  if (extractedArtist.length() == 0) extractedArtist = "Unknown Artist";
  if (extractedAlbum.length() == 0)  extractedAlbum = "Unknown Album";

  // WIPE the old text area completely so letters don't overlap
  tft.fillRect(0, y - 50, 240, 110, TFT_BLACK);

  tft.loadFont(Segoe_UI15);

  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);

  // Print Title (White)
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.drawString(extractedTitle.c_str(), (tft.width()/2), y - 30);

  // Print Artist (Light Grey)
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(extractedArtist.c_str(), (tft.width()/2), y); 

  // Print Album (Dark Grey)
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString(extractedAlbum.c_str(), (tft.width()/2), y + 30); 

  tft.setTextDatum(TL_DATUM);

  // Only unload the font if it actually successfully loaded earlier
  tft.unloadFont();
}