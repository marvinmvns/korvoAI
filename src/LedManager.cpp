#include "LedManager.h"

LedManager::LedManager() {
    _currentState = LED_IDLE;
    _audioLevel = 0;
    _hue = 0;
    _dist = 0;
    // Start with a Cyberpunk palette
    _targetPalette = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::Purple);
    _currentPalette = _targetPalette;
}

void LedManager::begin() {
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
    FastLED.setBrightness(80); // Um pouco mais brilhante para efeitos vividos
    FastLED.clear();
    FastLED.show();
}

void LedManager::setState(LedState state) {
    if (_currentState != state) {
        _currentState = state;
        // Reset or transition logic could go here
        // For example, flash white briefly on state change?
        // fill_solid(leds, LED_COUNT, CRGB(50,50,50)); 
    }
}

void LedManager::setAudioLevel(uint8_t level) {
    // Aggressive smoothing for smoother VU meter
    // If rising, go fast. If falling, go slow (gravity effect)
    if (level > _audioLevel) {
        _audioLevel = level; 
    } else {
        _audioLevel = (uint8_t)((_audioLevel * 10 + level) / 11);
    }
}

void LedManager::loop() {
    // 1. Blend palette smoothly towards target (Magical transitions)
    EVERY_N_MILLISECONDS(20) {
        nblendPaletteTowardPalette(_currentPalette, _targetPalette, 12);
    }

    // 2. Change palettes randomly in IDLE mode
    changePalettePeriodically();

    // 3. Run specific animation at ~60 FPS
    EVERY_N_MILLISECONDS(16) {
        switch (_currentState) {
            case LED_IDLE:       animIdle();       break;
            case LED_LISTENING:  animListening();  break;
            case LED_PROCESSING: animProcessing(); break;
            case LED_SPEAKING:   animSpeaking();   break;
            case LED_ERROR:      animError();      break;
        }
        
        // Add random dithering for "analog" feel
        FastLED.show();
    }
}

// ===========================================================================
// ANIMATIONS
// ===========================================================================

void LedManager::animIdle() {
    // "Aurora / Plasma"
    // Use Perlin Noise to generate smooth, organic movement
    
    uint8_t scale = 50;  // Scale of the noise
    _dist += 3;          // Speed of movement
    
    for (int i = 0; i < LED_COUNT; i++) {
        // Calculate 2D noise coordinate based on circular position
        // This makes the noise loop perfectly around the ring
        uint8_t angle = (i * 256) / LED_COUNT;
        uint16_t x = cos8(angle) * scale;
        uint16_t y = sin8(angle) * scale;
        
        // 3rd dimension is time (_dist)
        uint8_t noise = inoise16(x + _dist, y + _dist, _dist * 3) >> 8;
        
        // Map noise to palette color
        leds[i] = ColorFromPalette(_currentPalette, noise, 255, LINEARBLEND);
    }
}

void LedManager::animListening() {
    // "Magic Eye" - Rotating energy blob + Breathing
    fadeToBlackBy(leds, LED_COUNT, 40); // Leave trails
    
    // Rotating position
    static uint16_t pos16 = 0;
    pos16 += 400; // Speed
    uint8_t pos = (pos16 >> 8) % LED_COUNT;
    
    // Breathing brightness
    uint8_t breath = beatsin8(30, 100, 255);
    
    // Draw the "Eye"
    leds[pos] = ColorFromPalette(_currentPalette, millis() / 10, breath, LINEARBLEND);
    
    // Add "sparks" near the eye
    if (random8() < 40) {
        int r = (pos + random8(3)) % LED_COUNT;
        leds[r] += CRGB::White;
    }
}

void LedManager::animProcessing() {
    // "Neural Synapses" / "Data Rain"
    // Random pixels ignite and fade quickly
    
    fadeToBlackBy(leds, LED_COUNT, 20); // Fast fade
    
    if (random8() < 80) { // Frequent sparks
        int pos = random8(LED_COUNT);
        // Use random colors from the current magical palette
        CRGB color = ColorFromPalette(_currentPalette, random8(), 255, LINEARBLEND);
        // Make it very bright
        leds[pos] = color;
        // White core for the spark
        leds[pos] += CRGB(60,60,60);
    }
}

void LedManager::animSpeaking() {
    // "Hyper Voice"
    // Center-out VU meter, but using the dynamic palette for color
    
    fadeToBlackBy(leds, LED_COUNT, 80); // Clear somewhat fast
    
    // Map audio to number of pixels (0 to 6)
    // Center is between index 5 and 6
    uint8_t intensity = map(_audioLevel, 20, 255, 0, 7);
    
    // Color offset moves over time to make it "flow"
    uint8_t colorIndexBase = millis() / 5;
    
    for (int i = 0; i < intensity; i++) {
        int left = 5 - i;
        int right = 6 + i;
        
        // Color changes as it gets louder/wider
        CRGB color = ColorFromPalette(_currentPalette, colorIndexBase + (i * 15), 255, LINEARBLEND);
        
        if (left >= 0) leds[left] = color;
        if (right < LED_COUNT) leds[right] = color;
    }
    
    // Peak hold effect (occasional white tip)
    if (intensity >= 5 && random8() < 100) {
       if (5-intensity >= 0) leds[5-intensity] = CRGB::White;
       if (6+intensity < LED_COUNT) leds[6+intensity] = CRGB::White;
    }
}

void LedManager::animError() {
    // Glitchy Red
    fill_solid(leds, LED_COUNT, CRGB::Black);
    if ((millis() / 100) % 2 == 0) {
        // Random red pixels
        for(int i=0; i<3; i++) leds[random8(LED_COUNT)] = CRGB::Red;
    }
}

void LedManager::changePalettePeriodically() {
    if (_currentState != LED_IDLE) return;
    
    EVERY_N_SECONDS(10) {
        // Randomly pick a new magical palette
        switch (random8(6)) {
            case 0: _targetPalette = CRGBPalette16(CRGB::Blue, CRGB::Aqua, CRGB::Purple, CRGB::Pink); break; // Cyber
            case 1: _targetPalette = OceanColors_p; break; // Water
            case 2: _targetPalette = ForestColors_p; break; // Nature
            case 3: _targetPalette = HeatColors_p; break; // Fire
            case 4: _targetPalette = CRGBPalette16(CRGB::FairyLight, CRGB::Gold, CRGB::White, CRGB::Lavender); break; // Magic
            case 5: _targetPalette = PartyColors_p; break; // Fun
        }
    }
}

void LedManager::fadeAll(uint8_t amount) {
    for(int i = 0; i < LED_COUNT; i++) {
        leds[i].fadeToBlackBy(amount);
    }
}