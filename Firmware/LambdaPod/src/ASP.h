#ifndef ASP_H
#define ASP_H

#include <Arduino.h>
#include "AudioTools.h"
#include "AudioTools/Communication/A2DPStream.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "SDSpi.h"

class Asp {
public:
    Asp(A2DPStream* outputTarget, SDSpi& sharedSd);
    void PlayFile(String path);
    void Tick();
    
    // FIX: Add this public getter function
    bool IsPlaying() { return playing; }

private:
    A2DPStream* targetStream;
    SDSpi& sd;
    MP3DecoderHelix decoder;
    EncodedAudioStream decodedStream;
    bool playing; // This is the private variable we are exposing above
};

#endif