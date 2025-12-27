#include "AudioManager.h"

void AudioManager::setupMic() {
    i2s_driver_uninstall(SPEAK_I2S_NUMBER);
    delay(10);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_I2S_BCK_PIN,
        .ws_io_num = CONFIG_I2S_LRCK_PIN,
        .data_out_num = -1,
        .data_in_num = CONFIG_I2S_DATA_IN_PIN
    };
    pin_config.mck_io_num = I2S_PIN_NO_CHANGE;

    i2s_driver_install(SPEAK_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_set_pin(SPEAK_I2S_NUMBER, &pin_config);
    i2s_set_clk(SPEAK_I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

size_t AudioManager::readBytes(char* buffer, size_t length) {
    size_t bytesRead = 0;
    i2s_read(SPEAK_I2S_NUMBER, buffer, length, &bytesRead, portMAX_DELAY);
    return bytesRead;
}
