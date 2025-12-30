#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <Arduino.h>
#include <vector>

// OpenAI Chat Completions API
// Model: gpt-5-nano

struct ChatMessage {
    String role;    // "system", "user", "assistant"
    String content;
};

class LLMClient {
public:
    LLMClient(String apiKey);

    // Set system prompt
    void setSystemPrompt(String prompt);

    // Send user message and get response
    // Returns empty string on error
    String chat(String userMessage);

    // Clear conversation history (keeps system prompt)
    void clearHistory();

    // Set max tokens for response
    void setMaxTokens(int tokens);

private:
    String _apiKey;
    String _systemPrompt;
    std::vector<ChatMessage> _history;
    int _maxTokens = 150;
    static const int MAX_HISTORY = 10;  // Keep last N messages

    void trimHistory();
};

#endif
