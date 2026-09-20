#include "ASP.h"

Asp::Asp(A2DPStream* outputTarget, SDSpi& sharedSd) 
    : targetStream(outputTarget), 
      sd(sharedSd), 
      decodedStream(outputTarget, &decoder), 
      playing(false) {}

void Asp::PlayFile(String path) {
    if (playing) {
        sd.CurrentFile.close();
        playing = false;
    }

    sd.CurrentFile = SD.open(path);
    if (!sd.CurrentFile) {
        Serial.println("Decoder Error: Failed to open target file!");
        return;
    }

    Serial.println("[ASP] Connecting Helix decoder to active audio channel...");
    auto config = decodedStream.defaultConfig();
    decodedStream.begin(config); 

    playing = true;
    Serial.printf("Processor now decoding: %s\n", path.c_str());
}

void Asp::Tick() {
    if (playing && sd.CurrentFile) { // This will now resolve perfectly
        uint8_t buffer[512];
        int bytesRead = sd.CurrentFile.read(buffer, sizeof(buffer));
        
        if (bytesRead > 0) {
            int bytesWritten = decodedStream.write(buffer, bytesRead);
            if (bytesWritten == 0) {
                sd.CurrentFile.seek(sd.CurrentFile.position() - bytesRead);
            }
        } else {
            Serial.println("[ASP] Track finished streaming.");
            sd.CurrentFile.close();
            playing = false;
        }
    }
}