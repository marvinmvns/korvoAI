#include <M5Atom.h>
#include <driver/i2s.h>
#include "ConfigManager.h"
#include "AudioManager.h"
#include "OpenAIClient.h"
#include "ElevenLabsClient.h"
#include "AudioFileSourceHTTPSPost.h"
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

ConfigManager configManager;
AudioManager audioManager;
OpenAIClient *openai = NULL;
AudioGeneratorMP3 *mp3 = NULL;
AudioFileSourceHTTPSPost *source = NULL;
AudioOutputI2S *out = NULL;
bool isPlaying = false;

void cleanupAudio() {
    if (out) { out->stop(); delete out; out = NULL; }
    if (source) { delete source; source = NULL; }
    if (mp3) { delete mp3; mp3 = NULL; }
    i2s_driver_uninstall(I2S_NUM_0);
}

void setup() {
    M5.begin(true, false, true);
    delay(50);
    M5.dis.drawpix(0, 0xff0000);

    Serial.begin(115200);
    Serial.println("Atom Echo Assistant");

    WiFi.setSleep(false);

    // Factory reset if button held on boot
    M5.update();
    if (M5.Btn.isPressed()) {
        M5.dis.drawpix(0, 0xffffff);
        configManager.resetSettings();
        delay(2000);
        ESP.restart();
    }

    configManager.begin();
    openai = new OpenAIClient(configManager.getOpenAIKey());

    M5.dis.drawpix(0, 0x00ff00);
    Serial.println("Ready!");
}

void loop() {
    M5.update();

    // Handle playback
    if (isPlaying && mp3) {
        if (mp3->isRunning()) {
            if (!mp3->loop()) {
                mp3->stop();
                isPlaying = false;
                cleanupAudio();
                M5.dis.drawpix(0, 0x00ff00);
            }
        } else {
            isPlaying = false;
            cleanupAudio();
            M5.dis.drawpix(0, 0x00ff00);
        }
    }

    // Button press - start interaction
    if (M5.Btn.isPressed()) {
        if (isPlaying) {
            if (mp3) mp3->stop();
            isPlaying = false;
            cleanupAudio();
            delay(100);
        }

        M5.dis.drawpix(0, 0x0000ff);

        String text = openai->transcribeLiveAudio(10);
        Serial.println("You: " + text);

        if (text.indexOf("Error") == -1 && text.length() > 0) {
            String response = openai->chat(text);
            Serial.println("AI: " + response);

            if (response.indexOf("Error") == -1) {
                M5.dis.drawpix(0, 0xff00ff);
                Serial.printf("TTS heap: %d\n", ESP.getFreeHeap());

                ElevenLabsClient eleven(configManager.getElevenLabsKey(), configManager.getVoiceID());
                cleanupAudio();
                delay(50);

                source = eleven.getAudioSource(response);

                if (source && source->isOpen()) {
                    Serial.println("TTS stream open");
                    out = new AudioOutputI2S();
                    out->SetPinout(19, 33, 22);
                    out->SetGain(1.5);

                    mp3 = new AudioGeneratorMP3();
                    if (mp3->begin(source, out)) {
                        isPlaying = true;
                        Serial.println("Playing");
                    } else {
                        Serial.println("MP3 begin failed");
                        cleanupAudio();
                        M5.dis.drawpix(0, 0xff0000);
                    }
                } else {
                    Serial.println("TTS stream failed");
                    if (source) { delete source; source = NULL; }
                    M5.dis.drawpix(0, 0xff0000);
                }
            } else {
                Serial.println("Chat error");
                M5.dis.drawpix(0, 0xff0000);
            }
        } else {
            M5.dis.drawpix(0, 0xff0000);
        }

        while (M5.Btn.isPressed()) M5.update();
        delay(100);
        if (!isPlaying) M5.dis.drawpix(0, 0x00ff00);
    }
}
