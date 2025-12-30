# Repository Guidelines

## Project Structure & Module Organization
- `src/` contains all firmware source code (Arduino C++), including `main.cpp` and subsystem classes like `AudioManager`, `OpenAIClient`, and `ElevenLabsClient`.
- `platformio.ini` defines build environments (`m5stack-atom`, `esp32-korvo-v1_1`) and library dependencies.
- `.github/` holds repo-level automation/config (only Copilot instructions currently).

## Build, Test, and Development Commands
- `pio run -e m5stack-atom` builds the firmware for the M5Stack Atom Echo.
- `pio run -e m5stack-atom -t upload` flashes the device over USB.
- `pio device monitor` opens the serial monitor at 115200 baud.
- `pio run -e esp32-korvo-v1_1` builds for the KORVO v1.1 variant.

## Coding Style & Naming Conventions
- Use 4-space indentation and K&R-style braces to match existing C++ files in `src/`.
- Prefer `PascalCase` for class names and `camelCase` for functions/variables.
- Keep hardware constants in `BoardConfig.h` or relevant headers.
- No formatter or linter is configured; keep changes consistent with surrounding code.

## Testing Guidelines
- There is no automated test suite in this repository.
- Validate changes with a device smoke test: flash, open `pio device monitor`, and verify LED status + audio flow.
- If you add tests, consider PlatformIO’s `pio test` and document the target.

## Commit & Pull Request Guidelines
- Git history shows simple, sentence-style commit messages (e.g., “first commit”); keep messages short and descriptive.
- PRs should include: a brief summary, target board (`m5stack-atom` or `esp32-korvo-v1_1`), and any serial logs or videos if behavior changes.
- If user-facing behavior changes, update `README.md` and note any new configuration steps.

## Security & Configuration Tips
- Do not commit API keys. Use the WiFi portal setup or the optional hardcoded defines in `src/ConfigManager.cpp` for local testing only.
- Test on a 2.4GHz WiFi network; ESP32 doesn’t support 5GHz.
