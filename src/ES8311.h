#ifndef ES8311_H
#define ES8311_H

#include <Arduino.h>
#include <Wire.h>

// ES8311 Register Addresses
#define ES8311_REG_RESET            0x00
#define ES8311_REG_CLK_MANAGER1     0x01
#define ES8311_REG_CLK_MANAGER2     0x02
#define ES8311_REG_CLK_MANAGER3     0x03
#define ES8311_REG_CLK_MANAGER4     0x04
#define ES8311_REG_CLK_MANAGER5     0x05
#define ES8311_REG_CLK_MANAGER6     0x06
#define ES8311_REG_CLK_MANAGER7     0x07
#define ES8311_REG_CLK_MANAGER8     0x08
#define ES8311_REG_SDPIN            0x09
#define ES8311_REG_SDPOUT           0x0A
#define ES8311_REG_SYSTEM           0x0B
#define ES8311_REG_SYSTEM2          0x0C
#define ES8311_REG_ADC_HPF1         0x1C
#define ES8311_REG_ADC_HPF2         0x1D
#define ES8311_REG_ADC_CTRL         0x10
#define ES8311_REG_ADC_VOL          0x11
#define ES8311_REG_DAC_CTRL         0x12
#define ES8311_REG_DAC_VOL          0x13
#define ES8311_REG_GPIO             0x44
#define ES8311_REG_GP               0x45

class ES8311 {
public:
    ES8311(uint8_t i2cAddr = 0x18);

    bool begin(TwoWire *wire = &Wire);
    bool reset();

    // DAC (Speaker output)
    bool enableDAC(bool enable);
    bool setDACVolume(uint8_t volume);  // 0-255

    // ADC (Microphone input)
    bool enableADC(bool enable);
    bool setADCVolume(uint8_t volume);  // 0-255

    // General
    bool setMute(bool mute);
    bool setSampleRate(uint32_t sampleRate);

private:
    uint8_t _i2cAddr;
    TwoWire *_wire;

    bool writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
};

#endif // ES8311_H
