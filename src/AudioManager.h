#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23
#define SPEAK_I2S_NUMBER I2S_NUM_0

class AudioManager {
public:
    void setupMic();
    size_t readBytes(char* buffer, size_t length);
};

#endif
