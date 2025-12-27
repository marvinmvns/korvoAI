#include "OpenAIClient.h"
#include <M5Atom.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "AudioManager.h"

extern AudioManager audioManager;

OpenAIClient::OpenAIClient(String apiKey) : _apiKey(apiKey) {
    resetHistory();
}

void OpenAIClient::resetHistory() {
    _history.clear();
    _history.push_back({"system", "Você é uma assistente virtual simpática. Responda SEMPRE em no máximo 2 frases curtas. Seja direta e concisa. Português do Brasil."});
}

void OpenAIClient::pruneHistory() {
    size_t currentSize = 0;
    for (const auto& msg : _history) {
        currentSize += msg.content.length() + msg.role.length();
    }

    size_t limit = ESP.getFreeHeap() / 2;

    while (currentSize > limit && _history.size() > 1) {
        size_t removedSize = _history[1].content.length() + _history[1].role.length();
        _history.erase(_history.begin() + 1);
        currentSize -= removedSize;
    }
}

String OpenAIClient::transcribeLiveAudio(int maxDurationSeconds) {
    if (_apiKey.length() == 0) return "Error: No API Key";

    WiFiClientSecure client;
    client.setInsecure();

    if (!client.connect("api.openai.com", 443)) {
        return "Error: Connection Failed";
    }
    client.setNoDelay(true);

    const int sampleRate = 16000;
    size_t pcmSize = sampleRate * maxDurationSeconds * 2; // 16-bit mono
    size_t totalAudioSize = pcmSize + 44; // + WAV header

    String boundary = "----WebKitFormBoundary7MA4YWxk";
    String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\nContent-Type: audio/wav\r\n\r\n";
    String tail = "\r\n--" + boundary + "\r\nContent-Disposition: form-data; name=\"model\"\r\n\r\nwhisper-1\r\n--" + boundary + "--\r\n";

    client.println("POST /v1/audio/transcriptions HTTP/1.1");
    client.println("Host: api.openai.com");
    client.println("Authorization: Bearer " + _apiKey);
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println("Content-Length: " + String(head.length() + totalAudioSize + tail.length()));
    client.println();
    client.print(head);

    // WAV header
    uint8_t wav[44] = {'R','I','F','F', 0,0,0,0, 'W','A','V','E', 'f','m','t',' ', 16,0,0,0, 1,0, 1,0, 0,0,0,0, 0,0,0,0, 2,0, 16,0, 'd','a','t','a', 0,0,0,0};
    uint32_t fs = totalAudioSize - 8; memcpy(wav+4, &fs, 4);
    uint32_t sr = sampleRate; memcpy(wav+24, &sr, 4);
    uint32_t br = sampleRate * 2; memcpy(wav+28, &br, 4);
    uint32_t ds = pcmSize; memcpy(wav+40, &ds, 4);
    client.write(wav, 44);

    audioManager.setupMic();
    Serial.println("Recording...");

    size_t sent = 0;
    char buf[512];
    bool released = false;

    while (sent < pcmSize) {
        M5.update();
        if (!M5.Btn.isPressed() && !released) {
            released = true;
            M5.dis.drawpix(0, 0x000000);
        }

        if (!released) {
            size_t r = audioManager.readBytes(buf, sizeof(buf));
            if (r > 0) { client.write((uint8_t*)buf, r); sent += r; }
        } else {
            memset(buf, 0, sizeof(buf));
            while (sent < pcmSize) {
                size_t n = (pcmSize - sent < sizeof(buf)) ? pcmSize - sent : sizeof(buf);
                client.write((uint8_t*)buf, n);
                sent += n;
            }
        }
    }

    i2s_driver_uninstall(SPEAK_I2S_NUMBER);
    client.print(tail);

    unsigned long start = millis();
    while (client.connected() && !client.available() && millis() - start < 20000) delay(10);

    String result = "Error: Timeout";
    if (client.connected() && client.available()) {
        while (client.available()) { if (client.readStringUntil('\n') == "\r") break; }
        DynamicJsonDocument doc(512);
        deserializeJson(doc, client.readString());
        result = doc.containsKey("text") ? doc["text"].as<String>() : "Error: Parse";
    }
    client.stop();
    return result;
}

String OpenAIClient::chat(String prompt) {
    _history.push_back({"user", prompt});
    pruneHistory();

    size_t jsonSize = 256;
    for (const auto& msg : _history) jsonSize += msg.content.length() + msg.role.length() + 32;

    DynamicJsonDocument doc(jsonSize + 512);
    doc["model"] = "gpt-4o-mini";
    doc["stream"] = true;
    doc["max_tokens"] = 150;
    JsonArray messages = doc.createNestedArray("messages");
    for (const auto& msg : _history) {
        JsonObject m = messages.createNestedObject();
        m["role"] = msg.role;
        m["content"] = msg.content;
    }

    String body;
    serializeJson(doc, body);

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30000);

    if (!client.connect("api.openai.com", 443)) {
        if (_history.size() > 1) _history.pop_back();
        return "Error: Connection";
    }
    client.setNoDelay(true);

    Serial.println("Calling GPT (stream)...");
    client.println("POST /v1/chat/completions HTTP/1.1");
    client.println("Host: api.openai.com");
    client.println("Authorization: Bearer " + _apiKey);
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(body.length()));
    client.println();
    client.print(body);

    // Wait for response
    unsigned long start = millis();
    while (!client.available() && millis() - start < 30000) delay(10);

    // Skip HTTP headers
    bool headersDone = false;
    while (client.connected() && !headersDone) {
        String line = client.readStringUntil('\n');
        if (line == "\r") headersDone = true;
        if (line.startsWith("HTTP/") && line.indexOf("200") < 0) {
            Serial.println("GPT error: " + line);
            client.stop();
            if (_history.size() > 1) _history.pop_back();
            return "Error: API";
        }
    }

    // Parse SSE stream
    String content = "";
    start = millis();
    while (client.connected() && millis() - start < 30000) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            line.trim();
            if (line.startsWith("data: ")) {
                String data = line.substring(6);
                if (data == "[DONE]") break;
                DynamicJsonDocument chunk(512);
                if (deserializeJson(chunk, data) == DeserializationError::Ok) {
                    const char* delta = chunk["choices"][0]["delta"]["content"];
                    if (delta) {
                        content += delta;
                        Serial.print(delta);
                    }
                }
            }
            start = millis(); // Reset timeout on activity
        }
        delay(1);
    }
    Serial.println();

    client.stop();
    if (content.length() > 0) {
        _history.push_back({"assistant", content});
        return content;
    }

    if (_history.size() > 1) _history.pop_back();
    return "Error: Empty";
}
