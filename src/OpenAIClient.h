#ifndef OPENAI_CLIENT_H
#define OPENAI_CLIENT_H

#include <Arduino.h>
#include <vector>

struct ChatMessage {
    String role;
    String content;
};

class OpenAIClient {
public:
    OpenAIClient(String apiKey);
    String transcribeLiveAudio(int maxDurationSeconds);
    String chat(String prompt);
    void resetHistory();

private:
    String _apiKey;
    std::vector<ChatMessage> _history;
    void pruneHistory();
};

#endif
