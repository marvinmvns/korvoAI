#ifndef ELEVENLABS_CLIENT_H
#define ELEVENLABS_CLIENT_H

#include <Arduino.h>
#include "AudioFileSourceHTTPSPost.h"

class ElevenLabsClient {
public:
    ElevenLabsClient(String apiKey, String voiceID);
    AudioFileSourceHTTPSPost* getAudioSource(String text);

private:
    String _apiKey;
    String _voiceID;
};

#endif
