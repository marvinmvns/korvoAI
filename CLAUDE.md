# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 voice assistant firmware using OpenAI Realtime API for conversational AI. Supports two hardware platforms:
- **M5Stack Atom Echo**: Compact device with PDM microphone and single LED
- **ESP32-KORVO V1.1**: Development board with ES8311/ES7210 codecs and 12-LED ring

## Build Commands

```bash
# Build for M5Stack Atom Echo
pio run -e m5stack-atom

# Build for ESP32-KORVO V1.1
pio run -e esp32-korvo-v1_1

# Flash device
pio run -e m5stack-atom -t upload

# Serial monitor (115200 baud)
pio device monitor
```

## Architecture

### State Machine (main.cpp)
The application uses a VAD (Voice Activity Detection) state machine:
- `VAD_IDLE` → `VAD_SPEECH_START` → `VAD_RECORDING` → `VAD_IDLE`
- Two listening modes: Push-to-Talk (PTT) and Always-Listening (VAD-based)

### Audio Flow
1. **Recording**: `AudioManager` captures audio via I2S, applies VAD
2. **Streaming**: Audio chunks sent to OpenAI Realtime API via WebSocket (`OpenAIRealtimeClient`)
3. **Playback**: AI audio responses buffered in `AudioRingBuffer`, played through I2S

### Key Components
- `BoardConfig.h`: Hardware abstraction layer - all pin definitions and board-specific constants
- `AudioManager`: I2S setup (full-duplex), mic reading, VAD calibration
- `OpenAIRealtimeClient`: WebSocket client for OpenAI gpt-4o-realtime API
- `AudioRingBuffer`: Lock-free circular buffer for audio streaming
- `ConfigManager`: WiFi portal for API key configuration

## Memory Management

ESP32 has limited heap. Critical patterns:
- Audio buffers allocated with `malloc`/`new` - always check for NULL
- `playbackBuffer` and `preRollBuffer` are persistent ring buffers (64KB and 32KB)
- Audio callbacks write directly to ring buffers - no intermediate copies

## Hardware Abstraction

Board selection via `build_flags` in platformio.ini:
- `-D BOARD_M5STACK_ATOM_ECHO` or `-D BOARD_ESP32_KORVO_V1_1`
- Use `#if BOARD_HAS_*` preprocessor guards for board-specific code
- Pin mappings defined in `BoardConfig.h`

## Coding Style

- 4-space indentation, K&R braces
- PascalCase for classes, camelCase for functions/variables
- No formatter configured - match surrounding code

## Testing

No automated tests. Validate with device smoke test:
1. Flash firmware
2. Open `pio device monitor`
3. Verify LED status and audio flow
4. Serial commands: `STATUS`, `RECALIBRATE`
