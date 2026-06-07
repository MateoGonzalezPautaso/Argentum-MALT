#ifndef CLIENT_CHAT_HISTORY_H
#define CLIENT_CHAT_HISTORY_H

#include <cstdint>
#include <string>
#include <vector>

#include "../../common/messages.h"

struct ChatMessage {
    ChatMsgType type;
    std::string sender;
    std::string text;
    uint32_t timestamp;
};

class ChatHistory {
private:
    std::vector<ChatMessage> messages;
    bool new_message_flag = false;
    static constexpr size_t MAX_MESSAGES = 100;

public:
    void add_message(ChatMsgType type, const std::string& sender, const std::string& text);
    const std::vector<ChatMessage>& get_messages() const;
    void clear();
    bool has_new_message() const { return new_message_flag; }
    void consume_new_message() { new_message_flag = false; }
};

#endif
