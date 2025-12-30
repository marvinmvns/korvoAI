#ifndef TRANSCRIPTION_CLIENT_H
#define TRANSCRIPTION_CLIENT_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// OpenAI Realtime Transcription API
// Model: gpt-4o-mini-transcribe
// Audio format: PCM16 24kHz Mono

class TranscriptionClient {
public:
    TranscriptionClient(String apiKey);

    bool connect();
    void disconnect();
    void loop();

    // Audio input
    void sendAudio(uint8_t* data, size_t len);
    void commitAudio();  // Signal end of speech

    // State
    bool isConnected();
    bool isReady();

    // Events
    void onTranscriptionComplete(std::function<void(String)> callback);
    void onSpeechStarted(std::function<void()> callback);
    void onSpeechStopped(std::function<void()> callback);
    void onError(std::function<void(String)> callback);

private:
    String _apiKey;
    WebSocketsClient _webSocket;
    bool _ready = false;

    std::function<void(String)> _transcriptionCallback;
    std::function<void()> _speechStartedCallback;
    std::function<void()> _speechStoppedCallback;
    std::function<void(String)> _errorCallback;

    void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void sendSessionUpdate();
};

#endif
