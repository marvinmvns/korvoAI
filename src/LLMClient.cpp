#include "LLMClient.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

LLMClient::LLMClient(String apiKey) : _apiKey(apiKey) {
    _systemPrompt = "You are a helpful voice assistant. Respond naturally and concisely.";
}

void LLMClient::setSystemPrompt(String prompt) {
    _systemPrompt = prompt;
}

void LLMClient::setMaxTokens(int tokens) {
    _maxTokens = tokens;
}

void LLMClient::clearHistory() {
    _history.clear();
}

void LLMClient::trimHistory() {
    while (_history.size() > MAX_HISTORY) {
        _history.erase(_history.begin());
    }
}

String LLMClient::chat(String userMessage) {
    if (userMessage.length() == 0) return "";

    _history.push_back({"user", userMessage});
    trimHistory();

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30000);

    if (!client.connect("api.openai.com", 443)) {
        Serial.println("[LLM] Connect fail");
        return "";
    }

    // Build request for Responses API
    DynamicJsonDocument doc(4096);
    doc["model"] = "gpt-4.1-nano";
    doc["max_output_tokens"] = _maxTokens;
    doc["stream"] = true;
    doc["store"] = false;

    // Build input
    String input = _systemPrompt + "\n\n";
    for (const auto& msg : _history) {
        input += (msg.role == "user" ? "User: " : "Assistant: ") + msg.content + "\n";
    }
    doc["input"] = input;

    String body;
    serializeJson(doc, body);

    // Send request
    client.println("POST /v1/responses HTTP/1.1");
    client.println("Host: api.openai.com");
    client.println("Authorization: Bearer " + _apiKey);
    client.println("Content-Type: application/json");
    client.println("Accept: text/event-stream");
    client.println("Content-Length: " + String(body.length()));
    client.println("Connection: close");
    client.println();
    client.print(body);

    // Wait for response
    unsigned long timeout = millis() + 10000;
    while (client.connected() && !client.available()) {
        if (millis() > timeout) {
            client.stop();
            return "";
        }
        delay(10);
    }

    // Check status
    String status = client.readStringUntil('\n');
    if (status.indexOf("200") < 0) {
        while (client.available()) client.read();
        client.stop();
        return "";
    }

    // Skip headers
    while (client.connected()) {
        String h = client.readStringUntil('\n');
        h.trim();
        if (h.length() == 0) break;
    }

    // Read SSE stream
    String response = "";
    String line = "";
    unsigned long lastData = millis();

    while (client.connected() || client.available()) {
        if (client.available()) {
            char c = client.read();
            lastData = millis();

            if (c == '\n') {
                line.trim();
                if (line.length() > 0) {
                    // Skip hex chunk sizes
                    bool isHex = true;
                    for (unsigned int i = 0; i < line.length() && isHex; i++) {
                        if (!isxdigit(line.charAt(i))) isHex = false;
                    }

                    if (!isHex && line.startsWith("data: ")) {
                        String json = line.substring(6);
                        if (json == "[DONE]") break;

                        DynamicJsonDocument ev(4096);
                        if (!deserializeJson(ev, json)) {
                            const char* type = ev["type"];
                            if (type) {
                                if (strcmp(type, "response.output_text.delta") == 0 ||
                                    strcmp(type, "response.text.delta") == 0) {
                                    response += ev["delta"].as<String>();
                                }
                                else if (strcmp(type, "response.content_part.delta") == 0) {
                                    if (ev["delta"].containsKey("text")) {
                                        response += ev["delta"]["text"].as<String>();
                                    }
                                }
                                else if (strcmp(type, "response.completed") == 0 ||
                                         strcmp(type, "response.done") == 0) {
                                    if (ev.containsKey("response") && ev["response"].containsKey("output_text")) {
                                        response = ev["response"]["output_text"].as<String>();
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                line = "";
            } else if (c != '\r') {
                line += c;
            }
        } else {
            if (millis() - lastData > 15000) break;
            delay(1);
        }
        yield();
    }

    client.stop();

    if (response.length() > 0) {
        _history.push_back({"assistant", response});
        trimHistory();
    }

    return response;
}
