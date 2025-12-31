#include "ConfigManager.h"
#include <WiFiManager.h>
#include <Preferences.h>

// Hardcoded keys (optional - leave empty to use WiFiManager)
#define FORCE_OPENAI_KEY "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"
#define FORCE_ELEVEN_KEY "sk_b248c63661ca3cb746af5ff0f07b5a296652202311097be6"
#define FORCE_VOICE_ID "21m00Tcm4TlvDq8ikWAM"

Preferences prefs;

void ConfigManager::begin() {
    loadConfig();

    WiFiManager wm;
    WiFiManagerParameter p1("openai", "OpenAI Key", _openAIKey.c_str(), 100);
    WiFiManagerParameter p2("eleven", "ElevenLabs Key", _elevenLabsKey.c_str(), 100);
    WiFiManagerParameter p3("voice", "Voice ID", _voiceID.c_str(), 50);

    wm.addParameter(&p1);
    wm.addParameter(&p2);
    wm.addParameter(&p3);

    wm.setSaveParamsCallback([&]() {
        _openAIKey = p1.getValue();
        _elevenLabsKey = p2.getValue();
        _voiceID = p3.getValue();
        saveConfig();
    });

    if (!wm.autoConnect("KORVO_Setup")) {
        ESP.restart();
    }

    Serial.println("[WiFi] Connected");
}

void ConfigManager::loadConfig() {
    prefs.begin("korvo-cfg", true);
    _openAIKey = prefs.getString("openai", "");
    _elevenLabsKey = prefs.getString("eleven", "");
    _voiceID = prefs.getString("voice", "21m00Tcm4TlvDq8ikWAM");
    prefs.end();

    // Override with hardcoded if set
    if (strlen(FORCE_OPENAI_KEY) > 0) _openAIKey = FORCE_OPENAI_KEY;
    if (strlen(FORCE_ELEVEN_KEY) > 0) _elevenLabsKey = FORCE_ELEVEN_KEY;
    if (strlen(FORCE_VOICE_ID) > 0) _voiceID = FORCE_VOICE_ID;
}

void ConfigManager::saveConfig() {
    prefs.begin("korvo-cfg", false);
    prefs.putString("openai", _openAIKey);
    prefs.putString("eleven", _elevenLabsKey);
    prefs.putString("voice", _voiceID);
    prefs.end();
}

String ConfigManager::getOpenAIKey() { return _openAIKey; }
String ConfigManager::getElevenLabsKey() { return _elevenLabsKey; }
String ConfigManager::getVoiceID() { return _voiceID; }

void ConfigManager::resetSettings() {
    WiFiManager wm;
    wm.resetSettings();
    prefs.begin("korvo-cfg", false);
    prefs.clear();
    prefs.end();
}
