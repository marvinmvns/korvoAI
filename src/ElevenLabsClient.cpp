#include "ElevenLabsClient.h"
#include <ArduinoJson.h>

ElevenLabsClient::ElevenLabsClient(String apiKey, String voiceID) : _apiKey(apiKey), _voiceID(voiceID) {}

AudioFileSourceHTTPSPost* ElevenLabsClient::getAudioSource(String text) {
    // Truncate text to avoid 422 errors and reduce latency
    if (text.length() > 400) {
        int lastSpace = text.lastIndexOf(' ', 400);
        if (lastSpace > 200) text = text.substring(0, lastSpace) + "...";
        else text = text.substring(0, 400) + "...";
    }

    String url = "https://api.elevenlabs.io/v1/text-to-speech/" + _voiceID + "/stream?optimize_streaming_latency=4&output_format=mp3_22050_32";

    DynamicJsonDocument doc(512);
    doc["text"] = text;
    doc["model_id"] = "eleven_flash_v2_5";
    doc["voice_settings"]["stability"] = 0.35;
    doc["voice_settings"]["similarity_boost"] = 0.5;

    String payload;
    serializeJson(doc, payload);

    AudioFileSourceHTTPSPost *source = new AudioFileSourceHTTPSPost(url.c_str(), _apiKey.c_str(), payload.c_str());
    source->open();
    return source;
}
