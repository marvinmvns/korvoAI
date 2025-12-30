#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include "BoardConfig.h"
#include "ES8311.h"
#include "ES7210.h"

class AudioManager {
public:
    bool begin();
    void setupFullDuplex();
    size_t readBytes(char* buffer, size_t length);
    void enablePA(bool enable);
    void stopMic();
    void startMic();

    // Volume control
    void setVolume(uint8_t volume);
    void setMute(bool mute);

    // Noise calibration
    void calibrateNoise(int samples = 50);

private:
    ES8311 _codec;
    ES7210 _adc;
    bool _micRunning = false;
};

#endif
