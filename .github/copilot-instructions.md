# GitHub Copilot Instructions for Atom Echo Voice Assistant

## Project Overview
This is a PlatformIO project for the **M5Stack Atom Echo** (ESP32-PICO) functioning as a voice assistant. It integrates OpenAI (Whisper STT, ChatGPT) and ElevenLabs (TTS).

## Architecture & Data Flow
- **Entry Point:** `src/main.cpp` manages the application state (Idle, Recording, Processing, Playing).
- **Audio Input:** `AudioManager` captures audio via I2S (PDM mode) into a raw memory buffer.
- **Audio Output:** Uses `ESP8266Audio` library. `AudioGeneratorMP3` decodes streams from `AudioFileSourceHTTPSPost` and outputs to `AudioOutputI2S`.
- **Streaming:** `AudioFileSourceHTTPSPost` is a custom adapter that streams TTS audio directly from an HTTPS POST response to the audio player, avoiding full-file buffering.

## Critical Developer Workflows
- **Build & Upload:** Use PlatformIO commands.
  - Build: `platformio run`
  - Upload: `platformio run --target upload`
  - Monitor: `platformio device monitor` (Speed: 115200)
- **Dependencies:** Managed via `platformio.ini`. Key libs: `M5Atom`, `FastLED`, `ArduinoJson`, `ESP8266Audio`, `WiFiManager`.

## Coding Conventions & Patterns

### Memory Management (CRITICAL)
- **Heap is scarce.** The ESP32 has limited RAM.
- **Recording:** Audio buffers are allocated with `malloc`. **ALWAYS** `free()` the buffer immediately after the API call (e.g., after `openai.transcribeAudio`).
- **Playback:** Audio objects (`AudioGeneratorMP3`, `AudioFileSourceHTTPSPost`) are dynamically allocated (`new`) when needed and **MUST** be `delete`d when playback finishes or stops.
- **Check Allocations:** Always check if pointers are `NULL` after `malloc` or `new`.

### Audio Handling
- **Input (I2S):** Configured in `AudioManager.cpp`. Uses PDM mode (`I2S_MODE_PDM`) for the Atom Echo microphone.
- **Output (I2S):** Configured in `main.cpp` using `AudioOutputI2S`.
- **TTS Streaming:** Do not download TTS files to storage. Use `ElevenLabsClient::getAudioSource()` which returns a `AudioFileSourceHTTPSPost*` for direct streaming.

### API Integration
- **JSON:** Use `ArduinoJson` (`DynamicJsonDocument`) for constructing API payloads.
- **HTTPS:** Use `WiFiClientSecure` for all API calls.
- **Blocking:** Recording blocks the loop for a fixed duration. Playback is non-blocking (handled in `loop()` via `mp3->loop()`).

## Key Files
- `src/main.cpp`: Main state machine and loop.
- `src/AudioManager.cpp`: Low-level I2S recording logic.
- `src/AudioFileSourceHTTPSPost.cpp`: Custom stream reader for POST requests.
- `src/ElevenLabsClient.cpp`: TTS API wrapper.
