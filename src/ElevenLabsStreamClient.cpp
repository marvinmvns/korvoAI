#include "ElevenLabsStreamClient.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

ElevenLabsStreamClient::ElevenLabsStreamClient(String apiKey, String voiceId)
    : _apiKey(apiKey), _voiceId(voiceId) {
}

void ElevenLabsStreamClient::setVoiceId(String voiceId) {
    _voiceId = voiceId;
}

void ElevenLabsStreamClient::onAudioStart(std::function<void()> callback) {
    _audioStartCallback = callback;
}

void ElevenLabsStreamClient::onAudioComplete(std::function<void()> callback) {
    _audioCompleteCallback = callback;
}

void ElevenLabsStreamClient::onError(std::function<void(String)> callback) {
    _errorCallback = callback;
}

String ElevenLabsStreamClient::getLastError() {
    return _lastError;
}

bool ElevenLabsStreamClient::speak(String text, AudioRingBuffer* outputBuffer) {
    if (text.length() == 0 || !outputBuffer) {
        _lastError = "Invalid input";
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30000);

    String url = "/v1/text-to-speech/" + _voiceId + "/stream?output_format=pcm_24000";

    // Build request
    DynamicJsonDocument doc(1024);
    doc["text"] = text;
    doc["model_id"] = "eleven_flash_v2_5";

    JsonObject vs = doc.createNestedObject("voice_settings");
    vs["stability"] = 0.5;
    vs["similarity_boost"] = 0.75;

    String body;
    serializeJson(doc, body);

    if (!client.connect("api.elevenlabs.io", 443)) {
        _lastError = "Connect fail";
        if (_errorCallback) _errorCallback(_lastError);
        return false;
    }

    // Send request
    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: api.elevenlabs.io");
    client.println("xi-api-key: " + _apiKey);
    client.println("Content-Type: application/json");
    client.println("Accept: audio/pcm");
    client.println("Content-Length: " + String(body.length()));
    client.println("Connection: close");
    client.println();
    client.print(body);

    // Wait for response
    unsigned long timeout = millis() + 10000;
    while (client.connected() && !client.available()) {
        if (millis() > timeout) {
            _lastError = "Timeout";
            client.stop();
            return false;
        }
        delay(10);
    }

    // Check status
    String status = client.readStringUntil('\n');
    if (status.indexOf("200") < 0) {
        while (client.available()) client.read();
        _lastError = "HTTP error";
        client.stop();
        return false;
    }

    // Parse headers
    bool chunked = false;
    while (client.connected()) {
        String h = client.readStringUntil('\n');
        h.trim();
        if (h.length() == 0) break;
        if (h.indexOf("chunked") >= 0) chunked = true;
    }

    if (_audioStartCallback) _audioStartCallback();

    // Read audio data
    const int BUF_SIZE = 2048;
    uint8_t* buf = (uint8_t*)malloc(BUF_SIZE);
    if (!buf) {
        _lastError = "OOM";
        client.stop();
        return false;
    }

    size_t total = 0;
    size_t dropped = 0;
    unsigned long lastData = millis();

    while (client.connected() || client.available()) {
        if (client.available()) {
            int read;

            if (chunked) {
                String chunkLine = client.readStringUntil('\n');
                chunkLine.trim();
                if (chunkLine.length() == 0) continue;

                int chunkSize = strtol(chunkLine.c_str(), NULL, 16);
                if (chunkSize == 0) break;

                int remaining = chunkSize;
                while (remaining > 0 && (client.connected() || client.available())) {
                    int toRead = (remaining > BUF_SIZE) ? BUF_SIZE : remaining;
                    read = client.read(buf, toRead);
                    if (read > 0) {
                        size_t written = outputBuffer->write(buf, read);
                        if (written < (size_t)read) {
                            dropped += (read - written);
                        }
                        total += written;
                        remaining -= read;
                        lastData = millis();
                    } else {
                        delay(1);
                    }
                }
                client.readStringUntil('\n');
            } else {
                read = client.read(buf, BUF_SIZE);
                if (read > 0) {
                    size_t written = outputBuffer->write(buf, read);
                    if (written < (size_t)read) {
                        dropped += (read - written);
                    }
                    total += written;
                    lastData = millis();
                }
            }
        } else {
            if (millis() - lastData > 15000) break;
            delay(5);
        }
        yield();
    }

    if (dropped > 0) {
        Serial.printf("[TTS] WARNING: %d bytes dropped (buffer full)\n", dropped);
    }

    free(buf);
    client.stop();

    if (_audioCompleteCallback) _audioCompleteCallback();

    if (total == 0) {
        _lastError = "No audio";
        return false;
    }

    return true;
}
