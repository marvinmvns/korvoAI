#ifndef ELEVENLABS_STREAM_CLIENT_H
#define ELEVENLABS_STREAM_CLIENT_H

#include <Arduino.h>
#include "AudioRingBuffer.h"

// ElevenLabs Text-to-Speech Streaming API
// Model: eleven_flash_v2_5
// Output: PCM16 @ 24kHz

class ElevenLabsStreamClient {
public:
    ElevenLabsStreamClient(String apiKey, String voiceId);

    // Set voice ID
    void setVoiceId(String voiceId);

    // Stream TTS audio to buffer
    // Returns true if successful
    bool speak(String text, AudioRingBuffer* outputBuffer);

    // Events
    void onAudioStart(std::function<void()> callback);
    void onAudioComplete(std::function<void()> callback);
    void onError(std::function<void(String)> callback);

    // Get last error
    String getLastError();

private:
    String _apiKey;
    String _voiceId;
    String _lastError;

    std::function<void()> _audioStartCallback;
    std::function<void()> _audioCompleteCallback;
    std::function<void(String)> _errorCallback;
};

#endif
