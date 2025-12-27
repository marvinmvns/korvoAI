#ifndef AUDIO_FILE_SOURCE_HTTPS_POST_H
#define AUDIO_FILE_SOURCE_HTTPS_POST_H

#include <Arduino.h>
#include <AudioFileSource.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

class AudioFileSourceHTTPSPost : public AudioFileSource {
public:
    AudioFileSourceHTTPSPost(const char *url, const char *apiKey, const char *payload);
    virtual ~AudioFileSourceHTTPSPost();

    virtual bool open(const char *url) override { return false; } // Not used
    bool open(); // Custom open

    virtual uint32_t read(void *data, uint32_t len) override;
    virtual bool seek(int32_t pos, int seekMode) override;
    virtual bool close() override;
    virtual bool isOpen() override;
    virtual uint32_t getSize() override;
    virtual uint32_t getPos() override;

private:
    String _url;
    String _apiKey;
    String _payload;
    WiFiClientSecure *_client;
    bool _connected;
    bool _isChunked;
    int _chunkLen;
    uint32_t _pos;
};

#endif
