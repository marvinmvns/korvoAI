#include "TranscriptionClient.h"
#include <mbedtls/base64.h>

TranscriptionClient::TranscriptionClient(String apiKey) : _apiKey(apiKey) {
}

bool TranscriptionClient::connect() {
    _webSocket.beginSslWithCA("api.openai.com", 443, "/v1/realtime?intent=transcription", NULL, "wss");

    String auth = "Authorization: Bearer " + _apiKey;
    String beta = "OpenAI-Beta: realtime=v1";
    _webSocket.setExtraHeaders((auth + "\r\n" + beta).c_str());

    _webSocket.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        this->webSocketEvent(type, payload, length);
    });

    _webSocket.setReconnectInterval(5000);
    _ready = false;

    return true;
}

void TranscriptionClient::disconnect() {
    _webSocket.disconnect();
    _ready = false;
}

void TranscriptionClient::loop() {
    _webSocket.loop();
}

bool TranscriptionClient::isConnected() {
    return _webSocket.isConnected();
}

bool TranscriptionClient::isReady() {
    return _ready;
}

void TranscriptionClient::onTranscriptionComplete(std::function<void(String)> callback) {
    _transcriptionCallback = callback;
}

void TranscriptionClient::onSpeechStarted(std::function<void()> callback) {
    _speechStartedCallback = callback;
}

void TranscriptionClient::onSpeechStopped(std::function<void()> callback) {
    _speechStoppedCallback = callback;
}

void TranscriptionClient::onError(std::function<void(String)> callback) {
    _errorCallback = callback;
}

void TranscriptionClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            _ready = false;
            break;

        case WStype_CONNECTED:
            Serial.println("[WS] Connected");
            sendSessionUpdate();
            break;

        case WStype_TEXT: {
            DynamicJsonDocument doc(4096);
            if (deserializeJson(doc, payload, length)) return;

            const char* eventType = doc["type"];
            if (!eventType) return;

            if (strcmp(eventType, "transcription_session.updated") == 0 ||
                strcmp(eventType, "session.updated") == 0) {
                _ready = true;
                Serial.println("[WS] Ready");
            }
            else if (strcmp(eventType, "conversation.item.input_audio_transcription.completed") == 0) {
                if (doc.containsKey("transcript") && _transcriptionCallback) {
                    String text = doc["transcript"].as<String>();
                    if (text.length() > 0) _transcriptionCallback(text);
                }
            }
            else if (strcmp(eventType, "input_audio_buffer.transcription.completed") == 0 ||
                     strcmp(eventType, "transcription.completed") == 0) {
                String text = "";
                if (doc.containsKey("transcript")) text = doc["transcript"].as<String>();
                else if (doc.containsKey("text")) text = doc["text"].as<String>();
                if (text.length() > 0 && _transcriptionCallback) _transcriptionCallback(text);
            }
            else if (strcmp(eventType, "input_audio_buffer.speech_started") == 0) {
                if (_speechStartedCallback) _speechStartedCallback();
            }
            else if (strcmp(eventType, "input_audio_buffer.speech_stopped") == 0) {
                if (_speechStoppedCallback) _speechStoppedCallback();
            }
            else if (strcmp(eventType, "error") == 0) {
                if (_errorCallback) {
                    String msg = "Error";
                    if (doc.containsKey("error") && doc["error"].containsKey("message")) {
                        msg = doc["error"]["message"].as<String>();
                    }
                    _errorCallback(msg);
                }
            }
            break;
        }

        default:
            break;
    }
}

void TranscriptionClient::sendSessionUpdate() {
    DynamicJsonDocument doc(1024);
    doc["type"] = "transcription_session.update";

    JsonObject session = doc.createNestedObject("session");
    session["input_audio_format"] = "pcm16";

    JsonObject transcription = session.createNestedObject("input_audio_transcription");
    transcription["model"] = "gpt-4o-mini-transcribe";

    JsonObject turn_detection = session.createNestedObject("turn_detection");
    turn_detection["type"] = "server_vad";
    turn_detection["threshold"] = 0.5;
    turn_detection["prefix_padding_ms"] = 300;
    turn_detection["silence_duration_ms"] = 700;

    String json;
    serializeJson(doc, json);
    _webSocket.sendTXT(json);
}

void TranscriptionClient::sendAudio(uint8_t* data, size_t len) {
    if (!_webSocket.isConnected()) return;

    size_t b64Len = ((len + 2) / 3) * 4 + 1;
    char* b64Data = (char*)malloc(b64Len);

    if (b64Data) {
        size_t written;
        mbedtls_base64_encode((unsigned char*)b64Data, b64Len, &written, data, len);
        b64Data[written] = 0;

        String json = "{\"type\":\"input_audio_buffer.append\",\"audio\":\"";
        json += b64Data;
        json += "\"}";

        _webSocket.sendTXT(json);
        free(b64Data);
    }
}

void TranscriptionClient::commitAudio() {
    if (!_webSocket.isConnected()) return;
    _webSocket.sendTXT("{\"type\":\"input_audio_buffer.commit\"}");
}
