#include "AudioManager.h"

bool AudioManager::begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    delay(50);

    // Initialize PA pin
    pinMode(PA_ENABLE_PIN, OUTPUT);
    digitalWrite(PA_ENABLE_PIN, LOW);

    // Initialize ES8311 codec (DAC for speaker)
    if (!_codec.begin(&Wire)) {
        Serial.println("[Audio] ES8311 fail");
        return false;
    }

    // Setup I2S for MCLK before ES7210
    setupFullDuplex();
    delay(100);

    // Initialize ES7210 ADC (microphone)
    if (!_adc.begin(&Wire, ES7210_ADDR)) {
        Serial.println("[Audio] ES7210 fail");
        return false;
    }

    Serial.println("[Audio] Init OK");
    return true;
}

void AudioManager::setupFullDuplex() {
    // Uninstall existing drivers
    i2s_driver_uninstall(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_1);
    delay(10);

    // TX (Speaker) on I2S_NUM_0 - Optimized buffers
    i2s_config_t tx_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,  // Larger for smoother playback
        .use_apll = true,
        .tx_desc_auto_clear = true
    };

    i2s_pin_config_t tx_pins = {
        .bck_io_num = I2S_OUT_BCK_PIN,
        .ws_io_num = I2S_OUT_LRCK_PIN,
        .data_out_num = I2S_OUT_DATA_PIN,
        .data_in_num = -1
    };

    i2s_driver_install(I2S_NUM_0, &tx_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &tx_pins);

    // RX (Mic) on I2S_NUM_1
    i2s_config_t rx_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = true,
        .tx_desc_auto_clear = false
    };

    i2s_pin_config_t rx_pins = {
        .bck_io_num = I2S_IN_BCK_PIN,
        .ws_io_num = I2S_IN_LRCK_PIN,
        .data_out_num = -1,
        .data_in_num = I2S_IN_DATA_PIN
    };

    i2s_driver_install(I2S_NUM_1, &rx_config, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &rx_pins);

    // Enable Codec
    _codec.enableDAC(true);
    _codec.setMute(false);
    _codec.setDACVolume(85);
    enablePA(true);
    _micRunning = true;
}

void AudioManager::stopMic() {
    if (_micRunning) {
        i2s_stop(I2S_NUM_1);
        _micRunning = false;
    }
}

void AudioManager::startMic() {
    if (!_micRunning) {
        i2s_start(I2S_NUM_1);
        _micRunning = true;
    }
}

size_t AudioManager::readBytes(char* buffer, size_t length) {
    if (!_micRunning) return 0;

    // Read stereo and mix to mono
    const size_t CHUNK = 256;
    int16_t monoBuf[CHUNK];
    int16_t stereoBuf[CHUNK * 2];

    size_t total = 0;
    while (total < length) {
        size_t samples = (length - total) / 2;
        if (samples > CHUNK) samples = CHUNK;

        size_t stereoBytes = samples * 4;
        size_t read = 0;

        i2s_read(I2S_NUM_1, stereoBuf, stereoBytes, &read, 10);
        if (read == 0) break;

        size_t frames = read / 4;
        for (size_t i = 0; i < frames; i++) {
            monoBuf[i] = (stereoBuf[i * 2] + stereoBuf[i * 2 + 1]) / 2;
        }

        memcpy(buffer + total, monoBuf, frames * 2);
        total += frames * 2;

        if (read < stereoBytes) break;
    }

    return total;
}

void AudioManager::enablePA(bool enable) {
    digitalWrite(PA_ENABLE_PIN, enable ? HIGH : LOW);
}

void AudioManager::setVolume(uint8_t volume) {
    _codec.setDACVolume(volume);
}

void AudioManager::setMute(bool mute) {
    _codec.setMute(mute);
}

void AudioManager::calibrateNoise(int samples) {
    Serial.println("[Audio] Calibrating...");
    delay(samples * 20);
    Serial.println("[Audio] Done");
}
