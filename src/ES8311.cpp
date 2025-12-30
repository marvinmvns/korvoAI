#include "ES8311.h"

ES8311::ES8311(uint8_t i2cAddr) : _i2cAddr(i2cAddr), _wire(nullptr) {}

bool ES8311::begin(TwoWire *wire) {
    _wire = wire;

    // Check if device responds
    _wire->beginTransmission(_i2cAddr);
    if (_wire->endTransmission() != 0) {
        Serial.println("ES8311: Device not found");
        return false;
    }

    // ES8311 init sequence (BCLK as MCLK, per Korvo V1.1 schematic)
    writeReg(0x00, 0x1F);  // Reset all
    delay(20);
    writeReg(0x00, 0x00);  // Release reset, CSM off
    delay(20);

    // Clock configuration - SLAVE mode, use BCLK as MCLK
    writeReg(0x01, 0xBF);  // Enable clocks, MCLK source = BCLK
    writeReg(0x02, 0x18);  // BCLK*8 -> MCLK (LRCK * 256)
    writeReg(0x03, 0x10);  // LRCK div MSB (256)
    writeReg(0x04, 0x00);  // LRCK div LSB
    writeReg(0x05, 0x00);  // SCLK div = /4 (for 32-bit frame)
    writeReg(0x06, 0x03);  // BCLK ON, BCLK from MCLK
    writeReg(0x07, 0x00);  // OSR = 128
    writeReg(0x08, 0xFF);  // Enable all clocks

    // I2S format: 16-bit, I2S standard
    writeReg(0x09, 0x0C);  // SDP_IN: 16-bit
    writeReg(0x0A, 0x0C);  // SDP_OUT: 16-bit

    // System control
    writeReg(0x0B, 0x00);
    writeReg(0x0C, 0x00);

    // Power ON sequence
    writeReg(0x0D, 0x01);
    delay(10);
    writeReg(0x0E, 0x02);
    delay(10);
    writeReg(0x0F, 0x00);
    delay(50);
    writeReg(0x10, 0x1F);  // PGA gain
    writeReg(0x11, 0x00);  // ADC volume

    // DAC configuration
    writeReg(0x12, 0x00);  // DAC unmute
    writeReg(0x13, 0x10);  // DAC volume slightly reduced

    // Output mixer
    writeReg(0x14, 0x10);  // DAC->output enable

    // Output configuration
    writeReg(0x15, 0x00);
    writeReg(0x16, 0x00);
    writeReg(0x17, 0xBF);

    // Output driver
    writeReg(0x32, 0xC0);  // HP amp enable
    delay(50);

    // Enable state machine
    writeReg(0x00, 0x80);
    delay(100);

    // Unmute and set final volume
    writeReg(0x12, 0x00);
    writeReg(0x13, 0x00);

    Serial.println("ES8311: Initialized");
    return true;
}

bool ES8311::reset() {
    // Software reset
    if (!writeReg(ES8311_REG_RESET, 0x80)) {
        return false;
    }
    delay(10);
    writeReg(ES8311_REG_RESET, 0x00);
    return true;
}

bool ES8311::enableDAC(bool enable) {
    uint8_t reg = readReg(ES8311_REG_SYSTEM);
    if (enable) {
        reg |= 0x80;  // Enable DAC
    } else {
        reg &= ~0x80;  // Disable DAC
    }
    return writeReg(ES8311_REG_SYSTEM, reg);
}

bool ES8311::setDACVolume(uint8_t volume) {
    return writeReg(ES8311_REG_DAC_VOL, volume);
}

bool ES8311::enableADC(bool enable) {
    uint8_t reg = readReg(ES8311_REG_SYSTEM);
    if (enable) {
        reg |= 0x40;  // Enable ADC
    } else {
        reg &= ~0x40;  // Disable ADC
    }
    return writeReg(ES8311_REG_SYSTEM, reg);
}

bool ES8311::setADCVolume(uint8_t volume) {
    return writeReg(ES8311_REG_ADC_VOL, volume);
}

bool ES8311::setMute(bool mute) {
    uint8_t reg = readReg(ES8311_REG_DAC_CTRL);
    if (mute) {
        reg |= 0x08;  // Mute
    } else {
        reg &= ~0x08;  // Unmute
    }
    return writeReg(ES8311_REG_DAC_CTRL, reg);
}

bool ES8311::setSampleRate(uint32_t sampleRate) {
    // Configure clock dividers based on sample rate
    // Assuming MCLK = 256 * Fs
    uint8_t ratio;
    switch (sampleRate) {
        case 8000:
            ratio = 32;
            break;
        case 16000:
            ratio = 16;
            break;
        case 22050:
            ratio = 12;
            break;
        case 44100:
            ratio = 6;
            break;
        case 48000:
            ratio = 6;
            break;
        default:
            ratio = 16;  // Default to 16kHz
    }

    writeReg(ES8311_REG_CLK_MANAGER3, ratio);
    return true;
}

bool ES8311::writeReg(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

uint8_t ES8311::readReg(uint8_t reg) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom((uint8_t)_i2cAddr, (uint8_t)1);
    if (_wire->available()) {
        return _wire->read();
    }
    return 0;
}
