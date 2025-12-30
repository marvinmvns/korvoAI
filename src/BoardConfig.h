#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <Arduino.h>

// ===========================================================================
// ESP32-KORVO V1.1 Configuration (Only supported board)
// ===========================================================================

#define BOARD_NAME "ESP32-KORVO V1.1"

// I2C Bus (for ES8311 and ES7210 codecs)
#define I2C_SDA_PIN                 19
#define I2C_SCL_PIN                 32
#define I2C_FREQ                    400000

// ES8311 Codec I2C Address
#define ES8311_I2C_ADDR             0x18

// ES7210 ADC I2C Address
#define ES7210_I2C_ADDR             0x40

// I2S Output (ES8311 DAC - JST Speaker)
#define I2S_OUT_BCK_PIN             25
#define I2S_OUT_LRCK_PIN            22
#define I2S_OUT_DATA_PIN            13
#define I2S_OUT_MCLK_PIN            0

// I2S Input (ES7210 ADC - Microphone)
#define I2S_IN_BCK_PIN              27
#define I2S_IN_LRCK_PIN             26
#define I2S_IN_DATA_PIN             36
#define I2S_IN_MCLK_PIN             0

// Audio Configuration
#define AUDIO_SAMPLE_RATE           24000
#define AUDIO_BITS_PER_SAMPLE       I2S_BITS_PER_SAMPLE_16BIT

// Power Amplifier Control
#define PA_ENABLE_PIN               12

// LED Ring (WS2812)
#define LED_PIN                     33
#define LED_COUNT                   12

// ADC Buttons
#define BUTTON_ADC_PIN              39

// Button ADC Thresholds
#define BTN_PLAY_ADC_MIN            0
#define BTN_PLAY_ADC_MAX            300
#define BTN_SET_ADC_MIN             700
#define BTN_SET_ADC_MAX             1000
#define BTN_VOL_DOWN_ADC_MIN        1300
#define BTN_VOL_DOWN_ADC_MAX        1600
#define BTN_VOL_UP_ADC_MIN          1900
#define BTN_VOL_UP_ADC_MAX          2200
#define BTN_MODE_ADC_MIN            2500
#define BTN_MODE_ADC_MAX            2800
#define BTN_REC_ADC_MIN             3100
#define BTN_REC_ADC_MAX             3500

// Button IDs
enum KorvoButton {
    BTN_NONE = -1,
    BTN_PLAY = 0,
    BTN_SET = 1,
    BTN_VOL_DOWN = 2,
    BTN_VOL_UP = 3,
    BTN_MODE = 4,
    BTN_REC = 5
};

// I2S Configuration - Optimized for performance
#define I2S_DMA_BUF_COUNT           8
#define I2S_DMA_BUF_LEN             512

#endif // BOARD_CONFIG_H
