#include "SDSpi.h"

void SDSpi::ParseFiles() {
    FoundFiles = 0;
    File root = SD.open("/");
    if (!root) return;

    while (File file = root.openNextFile()) {
        if (!file.isDirectory()) {
            String name = file.name();
            if (name.endsWith(".mp3") || name.endsWith(".MP3")) {
                if (FoundFiles < MAX_FILES) {
                    FileList[FoundFiles] = name.startsWith("/") ? name : "/" + name;
                    FoundFiles++;
                }
            }
        }
        file.close();
    }
    root.close();
}

bool SDSpi::Init() {
    SPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        return false;
    } else {
        return true;
    }
    
    //this->ParseFiles();
    
}