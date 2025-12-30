#ifndef AUDIO_RING_BUFFER_H
#define AUDIO_RING_BUFFER_H

#include <Arduino.h>
#include <esp_heap_caps.h>

class AudioRingBuffer {
public:
    AudioRingBuffer(size_t size) {
        _size = size;
        _buffer = NULL;
        _inPsram = false;

        // Try PSRAM first (external SPI RAM)
        if (ESP.getPsramSize() > 0) {
            _buffer = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
            if (_buffer) {
                _inPsram = true;
                Serial.printf("[Buffer] Allocated %d KB in PSRAM\n", size/1024);
            }
        }

        // Fallback to internal RAM with smaller size
        if (_buffer == NULL) {
            size_t fallbackSize = 128 * 1024;  // 128KB max for internal RAM
            if (size > fallbackSize) {
                _size = fallbackSize;
                Serial.printf("[Buffer] PSRAM unavailable, using %d KB internal\n", fallbackSize/1024);
            }
            _buffer = (uint8_t*)malloc(_size);
        }

        _head = 0;
        _tail = 0;
        _count = 0;
        _mutex = xSemaphoreCreateMutex();
    }

    ~AudioRingBuffer() {
        if (_buffer) free(_buffer);
        vSemaphoreDelete(_mutex);
    }

    bool isAllocated() {
        return _buffer != NULL;
    }

    size_t write(const uint8_t* data, size_t len) {
        if (!_buffer) return 0;
        xSemaphoreTake(_mutex, portMAX_DELAY);
        
        size_t freeSpace = _size - _count;
        if (len > freeSpace) len = freeSpace;

        if (len == 0) {
            xSemaphoreGive(_mutex);
            return 0;
        }

        size_t firstChunk = _size - _head;
        if (len <= firstChunk) {
            memcpy(_buffer + _head, data, len);
            _head += len;
        } else {
            memcpy(_buffer + _head, data, firstChunk);
            memcpy(_buffer, data + firstChunk, len - firstChunk);
            _head = len - firstChunk;
        }

        if (_head >= _size) _head = 0;
        _count += len;

        xSemaphoreGive(_mutex);
        return len;
    }

    size_t read(uint8_t* data, size_t len) {
        if (!_buffer) return 0;
        xSemaphoreTake(_mutex, portMAX_DELAY);

        if (len > _count) len = _count;

        if (len == 0) {
            xSemaphoreGive(_mutex);
            return 0;
        }

        size_t firstChunk = _size - _tail;
        if (len <= firstChunk) {
            memcpy(data, _buffer + _tail, len);
            _tail += len;
        } else {
            memcpy(data, _buffer + _tail, firstChunk);
            memcpy(data, _buffer, len - firstChunk);
            _tail = len - firstChunk;
        }

        if (_tail >= _size) _tail = 0;
        _count -= len;

        xSemaphoreGive(_mutex);
        return len;
    }

    size_t available() {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        size_t c = _count;
        xSemaphoreGive(_mutex);
        return c;
    }

    void clear() {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        _head = 0;
        _tail = 0;
        _count = 0;
        xSemaphoreGive(_mutex);
    }

private:
    uint8_t* _buffer;
    size_t _size;
    volatile size_t _head;
    volatile size_t _tail;
    volatile size_t _count;
    SemaphoreHandle_t _mutex;
    bool _inPsram;
};

#endif
