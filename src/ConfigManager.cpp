#include "ConfigManager.h"
#include <WiFiManager.h>
#include <Preferences.h>

// --- CHAVES HARDCODED (Opcional) ---
// Preencha aqui para forçar as chaves via código, ignorando o que foi salvo no WiFiManager.
// Exemplo: #define FORCE_OPENAI_KEY "sk-proj-..."
//#define FORCE_OPENAI_KEY "" 
//#define FORCE_ELEVEN_KEY ""
// Voice: "Rachel" (21m00Tcm4TlvDq8ikWAM) - Reliable female voice
//#define FORCE_VOICE_ID ""
// -----------------------------------

Preferences preferences;

void ConfigManager::begin() {
    loadConfig();

    WiFiManager wm;

    // Custom parameters
    WiFiManagerParameter custom_openai_key("openai", "OpenAI API Key", _openAIKey.c_str(), 100);
    WiFiManagerParameter custom_elevenlabs_key("elevenlabs", "ElevenLabs API Key", _elevenLabsKey.c_str(), 100);
    WiFiManagerParameter custom_voice_id("voiceid", "ElevenLabs Voice ID", _voiceID.c_str(), 50);

    wm.addParameter(&custom_openai_key);
    wm.addParameter(&custom_elevenlabs_key);
    wm.addParameter(&custom_voice_id);

    // If we want to force config portal on button press (handled effectively by main loop or setup logic depending on requirement)
    // For now, autoConnect will try to connect or start portal if failed.
    // To manually start: wm.startConfigPortal("AtomEcho_Setup");

    wm.setSaveParamsCallback([&]() {
        _openAIKey = custom_openai_key.getValue();
        _elevenLabsKey = custom_elevenlabs_key.getValue();
        _voiceID = custom_voice_id.getValue();
        saveConfig();
        Serial.println("Params saved");
    });

    if (!wm.autoConnect("AtomEcho_Setup")) {
        Serial.println("Failed to connect and hit timeout");
        ESP.restart();
    }

    Serial.println("Connected to WiFi :)");
}

void ConfigManager::loadConfig() {
    preferences.begin("atomecho-cfg", true);
    _openAIKey = preferences.getString("openai", "");
    _elevenLabsKey = preferences.getString("eleven", "");
    // Default Voice: "Rachel" (Female, American - works well with Turbo v2.5 for PT-BR)
    _voiceID = preferences.getString("voice", "21m00Tcm4TlvDq8ikWAM"); 
    preferences.end();

    // Sobrescreve se houver chaves hardcoded
    if (String(FORCE_OPENAI_KEY) != "") _openAIKey = FORCE_OPENAI_KEY;
    if (String(FORCE_ELEVEN_KEY) != "") _elevenLabsKey = FORCE_ELEVEN_KEY;
    if (String(FORCE_VOICE_ID) != "") _voiceID = FORCE_VOICE_ID;
}

void ConfigManager::saveConfig() {
    preferences.begin("atomecho-cfg", false);
    preferences.putString("openai", _openAIKey);
    preferences.putString("eleven", _elevenLabsKey);
    preferences.putString("voice", _voiceID);
    preferences.end();
}

String ConfigManager::getOpenAIKey() { return _openAIKey; }
String ConfigManager::getElevenLabsKey() { return _elevenLabsKey; }
String ConfigManager::getVoiceID() { return _voiceID; }

void ConfigManager::resetSettings() {
     WiFiManager wm;
     wm.resetSettings();
     preferences.begin("atomecho-cfg", false);
     preferences.clear();
     preferences.end();
     Serial.println("Settings reset");
}
