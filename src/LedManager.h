#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <FastLED.h>
#include "BoardConfig.h"

enum LedState {
    LED_IDLE,
    LED_LISTENING,
    LED_PROCESSING,
    LED_SPEAKING,
    LED_ERROR
};

class LedManager {
public:
    LedManager();
    void begin();
    void loop();
    void setState(LedState state);
    void setAudioLevel(uint8_t level); // 0-255

private:
    CRGB leds[LED_COUNT];
    LedState _currentState;
    uint8_t _audioLevel;
    
    // Palette management for magical colors
    CRGBPalette16 _currentPalette;
    CRGBPalette16 _targetPalette;
    
    // Animation variables
    uint8_t _hue;
    uint16_t _dist; // Noise distance
    
    // Helpers
    void animIdle();
    void animListening();
    void animProcessing();
    void animSpeaking();
    void animError();
    
    // Internal
    void fadeAll(uint8_t amount);
    void changePalettePeriodically();
};

#endif