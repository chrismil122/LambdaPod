#ifndef SDSPI_H
#define SDSPI_H
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <SD.h>

extern TFT_eSPI tft;

#define SPI_SCLK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_SS   5

#define MAX_FILES 10
#define SD_CS 5

class SDSpi {
    public:
        void ParseFiles();
        bool Init();
        File CurrentFile;
        String CurrentFilePath;
    private:
        int FoundFiles;
        String FileList[MAX_FILES];
};

#endif