#include <Arduino.h>
#include "BoardConfig.h"
#include "AudioRingBuffer.h"

// Disable brownout detector
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <FastLED.h>
#include <driver/i2s.h>
#include <esp_wifi.h>
#include "ConfigManager.h"
#include "AudioManager.h"
#include "TranscriptionClient.h"
#include "LLMClient.h"
#include "ElevenLabsStreamClient.h"


// ===========================================================================
// Global Objects
// ===========================================================================
ConfigManager configManager;
AudioManager audioManager;
TranscriptionClient* transcriptionClient = NULL;
LLMClient* llmClient = NULL;
ElevenLabsStreamClient* ttsClient = NULL;

// Buffers - PCM 16kHz 16bit = 32KB/s
// PSRAM: 3MB buffer = ~90s | Internal: 128KB = 4s
AudioRingBuffer* playbackBuffer = NULL;
const size_t PLAYBACK_BUF_SIZE = 3 * 1024 * 1024;  // 3MB in PSRAM (4MB limit)

// Anti-echo cooldown
unsigned long cooldownUntil = 0;
const unsigned long COOLDOWN_MS = 1500;  // Ignore mic for 1.5s after speaking

// LED
CRGB leds[LED_COUNT];

// State machine
enum AppState { STATE_IDLE, STATE_LISTENING, STATE_PROCESSING, STATE_SPEAKING };
AppState currentState = STATE_IDLE;

// Flags
volatile bool isSpeaking = false;
String pendingText = "";
bool hasTranscription = false;

// Button state
KorvoButton lastBtn = BTN_NONE;
unsigned long btnTime = 0;

// ===========================================================================
// LED Functions
// ===========================================================================
void setLed(uint32_t color) {
    for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    }
    FastLED.show();
}

void animateLed() {
    static uint8_t hue = 0;
    if (currentState == STATE_LISTENING) {
        // Cyan pulse
        static uint8_t b = 128;
        static int8_t d = 3;
        b += d;
        if (b >= 250 || b <= 30) d = -d;
        for (int i = 0; i < LED_COUNT; i++) leds[i] = CRGB(0, b, b/2);
    } else if (currentState == STATE_PROCESSING) {
        // Orange pulse
        static uint8_t b2 = 128;
        static int8_t d2 = 4;
        b2 += d2;
        if (b2 >= 220 || b2 <= 60) d2 = -d2;
        for (int i = 0; i < LED_COUNT; i++) leds[i] = CRGB(b2, b2/2, 0);
    } else if (currentState == STATE_SPEAKING) {
        // Rainbow
        for (int i = 0; i < LED_COUNT; i++) leds[i] = CHSV(hue + i * 20, 255, 150);
        hue += 2;
    }
    FastLED.show();
}

// ===========================================================================
// Button Functions
// ===========================================================================
KorvoButton readButton() {
    int v = analogRead(BUTTON_ADC_PIN);
    if (v >= BTN_PLAY_ADC_MIN && v <= BTN_PLAY_ADC_MAX) return BTN_PLAY;
    if (v >= BTN_REC_ADC_MIN && v <= BTN_REC_ADC_MAX) return BTN_REC;
    if (v >= BTN_MODE_ADC_MIN && v <= BTN_MODE_ADC_MAX) return BTN_MODE;
    return BTN_NONE;
}

KorvoButton checkButton() {
    KorvoButton cur = readButton();
    if (cur != BTN_NONE && lastBtn == BTN_NONE) {
        lastBtn = cur;
        btnTime = millis();
        return BTN_NONE;
    }
    if (cur == BTN_NONE && lastBtn != BTN_NONE) {
        KorvoButton pressed = lastBtn;
        lastBtn = BTN_NONE;
        if (millis() - btnTime > 50) return pressed;
    }
    return BTN_NONE;
}

// ===========================================================================
// Audio Playback - PCM direct to I2S
// ===========================================================================
void playAudio() {
    if (!playbackBuffer || playbackBuffer->available() == 0) return;

    int total = playbackBuffer->available();
    Serial.printf("[Play] PCM %d bytes\n", total);

    uint8_t buf[2048];

    while (playbackBuffer->available() > 0) {
        size_t avail = playbackBuffer->available();
        size_t toRead = (avail > sizeof(buf)) ? sizeof(buf) : avail;
        playbackBuffer->read(buf, toRead);

        size_t written = 0;
        i2s_write(I2S_NUM_0, buf, toRead, &written, portMAX_DELAY);
    }

    // Flush
    delay(200);
    memset(buf, 0, sizeof(buf));
    size_t w = 0;
    i2s_write(I2S_NUM_0, buf, sizeof(buf), &w, 100);

    Serial.println("[Play] Done");
}

// ===========================================================================
// Process Pipeline
// ===========================================================================
void processPipeline(String text) {
    if (text.length() == 0) {
        currentState = STATE_IDLE;
        setLed(0x00ff00);
        return;
    }

    Serial.println("[User] " + text);
    currentState = STATE_PROCESSING;
    setLed(0xff8800);

    // Get LLM response
    String response = llmClient->chat(text);
    if (response.length() == 0) {
        Serial.println("[LLM] No response");
        currentState = STATE_IDLE;
        setLed(0x00ff00);
        return;
    }

    Serial.println("[AI] " + response);

    // Prepare for TTS
    isSpeaking = true;
    currentState = STATE_SPEAKING;
    setLed(0xff00ff);

    // Stop mic and disconnect WebSocket BEFORE TTS
    audioManager.stopMic();
    if (transcriptionClient) {
        transcriptionClient->disconnect();
    }

    // Clear buffer
    playbackBuffer->clear();

    // Stream TTS to buffer
    bool ok = ttsClient->speak(response, playbackBuffer);
    if (!ok) {
        Serial.println("[TTS] Failed");
    } else {
        // Play audio
        playAudio();
    }

    // Cleanup - wait for audio to fully finish
    delay(500);

    // Set cooldown to ignore echo from speaker
    cooldownUntil = millis() + COOLDOWN_MS;

    pendingText = "";
    hasTranscription = false;

    // Reconnect transcription
    audioManager.startMic();
    if (transcriptionClient) {
        transcriptionClient->connect();
    }

    isSpeaking = false;
    currentState = STATE_IDLE;
    setLed(0x00ff00);

    Serial.println("[Main] Ready (cooldown active)");
}

// ===========================================================================
// Setup
// ===========================================================================
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    delay(100);

    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
    FastLED.setBrightness(50);
    setLed(0xff0000);

    Serial.println("\n=== KORVO Voice Assistant ===");
    Serial.printf("Heap: %d\n", ESP.getFreeHeap());
    Serial.printf("PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

    if (!audioManager.begin()) {
        Serial.println("Audio fail!");
        while (1) delay(1000);
    }

    // WiFi setup
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.setSleep(false);

    // Check reset button
    if (readButton() == BTN_REC) {
        configManager.resetSettings();
        ESP.restart();
    }

    configManager.begin();

    // Initialize clients
    String openaiKey = configManager.getOpenAIKey();
    String elevenKey = configManager.getElevenLabsKey();
    String voiceId = configManager.getVoiceID();

    transcriptionClient = new TranscriptionClient(openaiKey);
    llmClient = new LLMClient(openaiKey);
    ttsClient = new ElevenLabsStreamClient(elevenKey, voiceId);

    llmClient->setSystemPrompt(
        "You are a helpful voice assistant. Respond naturally and concisely. "
        "Detect the user's language and respond in the same language. "
        "Keep responses short (1-2 sentences) for voice interaction."
    );
    llmClient->setMaxTokens(150);

    playbackBuffer = new AudioRingBuffer(PLAYBACK_BUF_SIZE);
    if (playbackBuffer->isAllocated()) {
        Serial.printf("Playback buffer: %d KB allocated\n", PLAYBACK_BUF_SIZE / 1024);
    } else {
        Serial.println("ERROR: Playback buffer allocation FAILED!");
    }

    // Transcription callbacks - with cooldown check
    transcriptionClient->onTranscriptionComplete([](String text) {
        if (isSpeaking || millis() < cooldownUntil) {
            Serial.println("[Main] Ignoring transcription (cooldown/speaking)");
            return;
        }
        pendingText = text;
        hasTranscription = true;
    });

    transcriptionClient->onSpeechStarted([]() {
        if (isSpeaking || millis() < cooldownUntil || currentState != STATE_IDLE) return;
        currentState = STATE_LISTENING;
    });

    transcriptionClient->onSpeechStopped([]() {
        if (isSpeaking || millis() < cooldownUntil) return;
    });

    // Calibrate and connect
    delay(500);
    audioManager.calibrateNoise(20);

    Serial.println("Connecting...");
    if (transcriptionClient->connect()) {
        Serial.println("Ready!");
        setLed(0x00ff00);
    } else {
        Serial.println("Connection failed");
        setLed(0xff0000);
    }

    currentState = STATE_IDLE;
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
}

// ===========================================================================
// Main Loop - Optimized
// ===========================================================================
void loop() {
    // Skip everything during playback
    if (isSpeaking) {
        animateLed();
        delay(20);
        return;
    }

    // Process WebSocket
    if (transcriptionClient) {
        transcriptionClient->loop();
    }

    // Check for completed transcription
    if (hasTranscription && pendingText.length() > 0) {
        String text = pendingText;
        pendingText = "";
        hasTranscription = false;
        processPipeline(text);
        return;
    }

    // State handling
    switch (currentState) {
        case STATE_IDLE:
        case STATE_LISTENING:
            // Stream audio to transcription (skip during cooldown)
            if (millis() >= cooldownUntil) {
                uint8_t buf[512];
                size_t r = audioManager.readBytes((char*)buf, sizeof(buf));
                if (r > 0 && transcriptionClient && transcriptionClient->isConnected()) {
                    transcriptionClient->sendAudio(buf, r);
                }
            }
            animateLed();
            break;

        case STATE_PROCESSING:
        case STATE_SPEAKING:
            animateLed();
            break;
    }

    // Button handling
    KorvoButton btn = checkButton();
    if (btn == BTN_MODE && currentState == STATE_IDLE) {
        // Toggle mode or other action
        setLed(0x0000ff);
        delay(200);
        setLed(0x00ff00);
    }

    // Small delay to prevent CPU hogging
    delay(1);
}
