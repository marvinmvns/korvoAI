#ifndef ES7210_H
#define ES7210_H

#include <Arduino.h>
#include <Wire.h>

// ES7210 I2C Address
#define ES7210_ADDR 0x40

// ES7210 Register Addresses
#define ES7210_RESET_REG00          0x00
#define ES7210_CLOCK_OFF_REG01      0x01
#define ES7210_MAINCLK_REG02        0x02
#define ES7210_MASTER_CLK_REG03     0x03
#define ES7210_LRCK_DIVH_REG04      0x04
#define ES7210_LRCK_DIVL_REG05      0x05
#define ES7210_POWER_DOWN_REG06     0x06
#define ES7210_OSR_REG07            0x07
#define ES7210_MODE_CONFIG_REG08    0x08
#define ES7210_TIME_CONTROL0_REG09  0x09
#define ES7210_TIME_CONTROL1_REG0A  0x0A
#define ES7210_SDP_INTERFACE1_REG11 0x11
#define ES7210_SDP_INTERFACE2_REG12 0x12
#define ES7210_ADC_AUTOMUTE_REG13   0x13
#define ES7210_ADC34_MUTEFLAG_REG14 0x14
#define ES7210_ADC12_MUTEFLAG_REG15 0x15
#define ES7210_ALC_SEL_REG16        0x16
#define ES7210_ALC_COM_CFG1_REG17   0x17
#define ES7210_ALC_COM_CFG2_REG18   0x18
#define ES7210_ADC1_MAX_GAIN_REG1A  0x1A
#define ES7210_ADC2_MAX_GAIN_REG1B  0x1B
#define ES7210_ADC3_MAX_GAIN_REG1C  0x1C
#define ES7210_ADC4_MAX_GAIN_REG1D  0x1D
#define ES7210_ADC34_HPF2_REG20     0x20
#define ES7210_ADC34_HPF1_REG21     0x21
#define ES7210_ADC12_HPF2_REG22     0x22
#define ES7210_ADC12_HPF1_REG23     0x23
#define ES7210_ANALOG_REG40         0x40
#define ES7210_MIC12_BIAS_REG41     0x41
#define ES7210_MIC34_BIAS_REG42     0x42
#define ES7210_MIC1_GAIN_REG43      0x43
#define ES7210_MIC2_GAIN_REG44      0x44
#define ES7210_MIC3_GAIN_REG45      0x45
#define ES7210_MIC4_GAIN_REG46      0x46
#define ES7210_MIC1_POWER_REG47     0x47
#define ES7210_MIC2_POWER_REG48     0x48
#define ES7210_MIC3_POWER_REG49     0x49
#define ES7210_MIC4_POWER_REG4A     0x4A
#define ES7210_MIC12_POWER_REG4B    0x4B
#define ES7210_MIC34_POWER_REG4C    0x4C

class ES7210 {
public:
    ES7210() : _wire(nullptr) {}

    bool begin(TwoWire *wire = &Wire, uint8_t addr = ES7210_ADDR) {
        _wire = wire;
        _addr = addr;

        // Check if device is present
        _wire->beginTransmission(_addr);
        if (_wire->endTransmission() != 0) {
            Serial.println("ES7210: Device not found!");
            return false;
        }

        Serial.println("ES7210: Device found, initializing...");

        // Software reset
        writeReg(ES7210_RESET_REG00, 0xFF);
        delay(50);
        writeReg(ES7210_RESET_REG00, 0x32);
        delay(50);

        // Simple configuration - minimum registers
        writeReg(0x01, 0x3F);  // Clock off initially
        writeReg(0x00, 0x41);  // Slave mode, single speed
        writeReg(0x06, 0x00);  // Power on
        writeReg(0x07, 0x20);  // OSR = 32
        writeReg(0x09, 0x30);  // Time control 0
        writeReg(0x0A, 0x30);  // Time control 1

        // I2S format: 16-bit, I2S mode
        writeReg(0x11, 0x30);

        // Analog power
        writeReg(0x40, 0x42);
        delay(100);

        // MIC bias
        writeReg(0x41, 0x70);
        writeReg(0x42, 0x70);

        // MIC gain - maximum (0x1E = 37.5dB)
        writeReg(0x43, 0x1E);
        writeReg(0x44, 0x1E);
        writeReg(0x45, 0x1E);
        writeReg(0x46, 0x1E);

        // Power up mics
        writeReg(0x47, 0x08);
        writeReg(0x48, 0x08);
        writeReg(0x49, 0x08);
        writeReg(0x4A, 0x08);
        writeReg(0x4B, 0x0F);
        writeReg(0x4C, 0x0F);

        // Enable clocks
        writeReg(0x01, 0x00);

        delay(200);

        // Debug
        Serial.printf("ES7210: REG00=0x%02X, REG11=0x%02X, REG40=0x%02X\n",
            readReg(0x00), readReg(0x11), readReg(0x40));

        Serial.println("ES7210: Initialized");
        return true;
    }

    void setGain(uint8_t gain) {
        // Gain range: 0-0x1F (0dB to +37.5dB)
        if (gain > 0x1F) gain = 0x1F;
        writeReg(ES7210_MIC1_GAIN_REG43, gain);
        writeReg(ES7210_MIC2_GAIN_REG44, gain);
        writeReg(ES7210_MIC3_GAIN_REG45, gain);
        writeReg(ES7210_MIC4_GAIN_REG46, gain);
    }

private:
    TwoWire *_wire;
    uint8_t _addr;

    void writeReg(uint8_t reg, uint8_t val) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->write(val);
        _wire->endTransmission();
        delay(1);  // Small delay for register to settle
    }

    uint8_t readReg(uint8_t reg) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(_addr, (uint8_t)1);
        return _wire->read();
    }
};

#endif // ES7210_H
