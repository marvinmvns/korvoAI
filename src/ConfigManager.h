#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>


class ConfigManager {
public:
    void begin();
    String getOpenAIKey();
    String getElevenLabsKey();
    String getVoiceID();
    void resetSettings();

private:
    String _openAIKey;
    String _elevenLabsKey;
    String _voiceID;
    
    void loadConfig();
    void saveConfig();
};

#endif
