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
#include "LedManager.h"
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
const unsigned long COOLDOWN_MS = 300;  // Reduced to 300ms for faster turn-taking

// LED Manager
LedManager ledManager;

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
// Button Functions
// ===========================================================================
KorvoButton readButton() {
    int v = analogRead(BUTTON_ADC_PIN);
    if (v >= BTN_SET_ADC_MIN && v <= BTN_SET_ADC_MAX) return BTN_SET;
    if (v >= BTN_VOL_UP_ADC_MIN && v <= BTN_VOL_UP_ADC_MAX) return BTN_VOL_UP;
    if (v >= BTN_VOL_DOWN_ADC_MIN && v <= BTN_VOL_DOWN_ADC_MAX) return BTN_VOL_DOWN;
    if (v >= BTN_PLAY_ADC_MIN && v <= BTN_PLAY_ADC_MAX) return BTN_PLAY;
    if (v >= BTN_MODE_ADC_MIN && v <= BTN_MODE_ADC_MAX) return BTN_MODE;
    if (v >= BTN_REC_ADC_MIN && v <= BTN_REC_ADC_MAX) return BTN_REC;
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
// Audio Playback - Optimized for Streaming
// ===========================================================================
void playAudio() {
    if (!playbackBuffer) return;

    Serial.println("[Play] Start streaming");
    
    uint8_t buf[2048];
    unsigned long startTime = millis();
    bool buffering = true;

    while (isSpeaking || playbackBuffer->available() > 0) {
        size_t avail = playbackBuffer->available();
        
        // Initial buffering to prevent stutter (need at least 20KB to start)
        if (buffering && avail < 20 * 1024 && isSpeaking) {
            delay(10);
            if (millis() - startTime > 5000) buffering = false; // Safety timeout
            continue;
        }
        buffering = false;

        if (avail == 0) {
            if (!isSpeaking) break;
            delay(5);
            continue;
        }

        size_t toRead = (avail > sizeof(buf)) ? sizeof(buf) : avail;
        playbackBuffer->read(buf, toRead);

        // Calculate audio level for LED animation
        int16_t* samples = (int16_t*)buf;
        size_t sampleCount = toRead / 2;
        int16_t maxVal = 0;
        for (size_t i = 0; i < sampleCount; i += 8) { // Optimized check
            int16_t val = abs(samples[i]);
            if (val > maxVal) maxVal = val;
        }
        uint8_t ledLevel = map(constrain(maxVal, 0, 15000), 0, 15000, 0, 255);
        ledManager.setAudioLevel(ledLevel);
        ledManager.loop();

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
        ledManager.setState(LED_IDLE);
        return;
    }

    Serial.println("[User] " + text);
    currentState = STATE_PROCESSING;
    ledManager.setState(LED_PROCESSING);

    // Get LLM response
    String response = llmClient->chat(text);
    if (response.length() == 0) {
        Serial.println("[LLM] No response");
        currentState = STATE_IDLE;
        ledManager.setState(LED_IDLE);
        return;
    }

    Serial.println("[AI] " + response);

    // Prepare for TTS
    isSpeaking = true;
    currentState = STATE_SPEAKING;
    ledManager.setState(LED_SPEAKING);

    // Stop mic and disconnect WebSocket BEFORE TTS
    audioManager.stopMic();
    if (transcriptionClient) {
        transcriptionClient->disconnect();
    }

    // Clear buffer
    playbackBuffer->clear();

    // Task to play audio while it downloads
    xTaskCreate([](void* p) {
        playAudio();
        vTaskDelete(NULL);
    }, "play_task", 4096, NULL, 5, NULL);

    // Stream TTS to buffer (now non-blocking to the player task)
    bool ok = ttsClient->speak(response, playbackBuffer);
    
    // Signal end of download
    isSpeaking = false;

    if (!ok) {
        Serial.println("[TTS] Failed");
    }

    // Wait for playback task to finish (isSpeaking is false, buffer will drain)
    while (playbackBuffer->available() > 0) {
        delay(50); // Check faster
    }

    // Quick Cleanup
    delay(50); // Minimal settling time for speaker
    cooldownUntil = millis() + COOLDOWN_MS;
    pendingText = "";
    hasTranscription = false;

    // Reconnect transcription IMMEDIATELY
    audioManager.startMic();
    if (transcriptionClient) {
        // If already connected, this is a no-op or quick reset
        transcriptionClient->connect();
    }

    currentState = STATE_IDLE;
    ledManager.setState(LED_IDLE);
    Serial.println("[Main] Ready (fast turn-taking)");
}


// ===========================================================================
// Setup
// ===========================================================================
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    // Don't wait for Serial if not connected (standalone mode)
    for(int i = 0; i < 20 && !Serial; i++) {
        delay(50);
    }

    ledManager.begin();
    ledManager.setState(LED_ERROR); // Temporary RED during init

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
        ledManager.setState(LED_IDLE);
    } else {
        Serial.println("Connection failed");
        ledManager.setState(LED_ERROR);
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
        ledManager.loop(); // Keep animating during blocking playback calls if any
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
            break;

        case STATE_PROCESSING:
        case STATE_SPEAKING:
            break;
    }

    // Update LED animations based on current state
    if (currentState == STATE_LISTENING) ledManager.setState(LED_LISTENING);
    else if (currentState == STATE_PROCESSING) ledManager.setState(LED_PROCESSING);
    else if (currentState == STATE_SPEAKING) ledManager.setState(LED_SPEAKING);
    else ledManager.setState(LED_IDLE);

    ledManager.loop();

    // Button handling
    KorvoButton btn = checkButton();
    if (btn == BTN_MODE && currentState == STATE_IDLE) {
        // Toggle mode or other action
        ledManager.setState(LED_PROCESSING); // Visual feedback
        delay(200);
        ledManager.setState(LED_IDLE);
    }

    // Small delay to prevent CPU hogging
    delay(1);
}
