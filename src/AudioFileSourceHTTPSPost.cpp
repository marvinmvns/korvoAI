#include "AudioFileSourceHTTPSPost.h"

AudioFileSourceHTTPSPost::AudioFileSourceHTTPSPost(const char *url, const char *apiKey, const char *payload) {
    _url = url;
    _apiKey = apiKey;
    _payload = payload;
    _client = new WiFiClientSecure();
    _client->setInsecure();
    _client->setTimeout(15000);
    _connected = false;
    _isChunked = false;
    _chunkLen = 0;
    _pos = 0;
}

AudioFileSourceHTTPSPost::~AudioFileSourceHTTPSPost() {
    close();
    if (_client) delete _client;
}

bool AudioFileSourceHTTPSPost::open() {
    int slash = _url.indexOf("/", 8);
    String host = _url.substring(8, slash);
    String path = _url.substring(slash);

    Serial.println("Connecting to " + host);
    if (!_client->connect(host.c_str(), 443, 10000)) {
        Serial.println("Connection failed");
        return false;
    }

    _client->setNoDelay(true);
    _client->println("POST " + path + " HTTP/1.1");
    _client->println("Host: " + host);
    _client->println("xi-api-key: " + _apiKey);
    _client->println("Content-Type: application/json");
    _client->println("Content-Length: " + String(_payload.length()));
    _client->println();
    _client->print(_payload);
    _payload = "";

    unsigned long timeout = millis();
    while (!_client->available()) {
        if (!_client->connected() || millis() - timeout > 15000) return false;
        delay(10);
    }

    bool ok = false;
    String status = "";
    while (_client->connected()) {
        String line = _client->readStringUntil('\n');
        if (status.length() == 0) status = line;
        if (line.startsWith("HTTP/1.") && line.indexOf("200") > 0) ok = true;
        if (line.indexOf("chunked") >= 0) _isChunked = true;
        if (line == "\r") {
            if (ok) {
                Serial.println("Stream OK");
                _connected = true;
                _chunkLen = 0;
                return true;
            }
            Serial.println("HTTP error: " + status);
            return false;
        }
    }
    Serial.println("Connection closed");
    return false;
}

uint32_t AudioFileSourceHTTPSPost::read(void *data, uint32_t len) {
    if (!_connected || !_client->connected()) return 0;

    const uint32_t kTimeout = 5000;

    if (!_isChunked) {
        int avail = _client->available();
        if (avail == 0) {
            unsigned long start = millis();
            while (_client->available() == 0) {
                if (!_client->connected()) { _connected = false; return 0; }
                if (millis() - start > kTimeout) { _connected = false; return 0; }
                delay(1);
            }
            avail = _client->available();
        }
        int r = _client->read((uint8_t*)data, min((int)len, avail));
        _pos += r;
        return r;
    }

    // Chunked
    uint8_t *out = (uint8_t*)data;
    uint32_t total = 0;

    while (total < len) {
        if (_chunkLen == 0) {
            unsigned long start = millis();
            while (_client->available() == 0) {
                if (!_client->connected()) { _connected = false; _pos += total; return total; }
                if (millis() - start > kTimeout) { _pos += total; return total; }
                delay(1);
            }

            String line = _client->readStringUntil('\n');
            line.trim();
            if (line.length() == 0) { line = _client->readStringUntil('\n'); line.trim(); }
            if (line.length() == 0) break;

            long sz = strtol(line.c_str(), NULL, 16);
            if (sz == 0) { _connected = false; _pos += total; return total; }
            _chunkLen = sz;
        }

        int toRead = min((int)(len - total), _chunkLen);
        int avail = _client->available();

        if (avail == 0) {
            unsigned long start = millis();
            while (_client->available() == 0) {
                if (!_client->connected()) { _connected = false; _pos += total; return total; }
                if (millis() - start > kTimeout) { _pos += total; return total; }
                delay(1);
            }
            avail = _client->available();
        }

        if (toRead > avail) toRead = avail;
        int r = _client->read(out + total, toRead);
        if (r <= 0) break;

        total += r;
        _chunkLen -= r;

        if (_chunkLen == 0) {
            unsigned long start = millis();
            while (_client->available() < 2 && _client->connected() && millis() - start < 1000) delay(1);
            if (_client->available() >= 2) { char t[2]; _client->readBytes(t, 2); }
        }
    }
    _pos += total;
    return total;
}

bool AudioFileSourceHTTPSPost::seek(int32_t pos, int seekMode) { return false; }
bool AudioFileSourceHTTPSPost::close() { if (_client->connected()) _client->stop(); _connected = false; return true; }
bool AudioFileSourceHTTPSPost::isOpen() { return _connected; }
uint32_t AudioFileSourceHTTPSPost::getSize() { return 0; }
uint32_t AudioFileSourceHTTPSPost::getPos() { return _pos; }
