#include "chat_history.h"

#include <algorithm>
#include <utility>

#include <SDL2/SDL.h>

void ChatHistory::add_message(ChatMsgType type, const std::string& sender,
                              const std::string& text) {
    uint32_t timestamp = static_cast<uint32_t>(SDL_GetTicks());

    std::size_t start = 0;
    while (true) {
        std::size_t nl = text.find('\n', start);
        std::string line = text.substr(start, nl == std::string::npos ? nl : nl - start);
        messages.push_back({type, sender, std::move(line), timestamp});
        if (nl == std::string::npos)
            break;
        start = nl + 1;
    }
    new_message_flag = true;

    if (messages.size() > MAX_MESSAGES) {
        messages.erase(messages.begin(),
                       messages.begin() + static_cast<long>(messages.size() - MAX_MESSAGES));
    }
}

const std::vector<ChatMessage>& ChatHistory::get_messages() const { return messages; }

void ChatHistory::clear() { messages.clear(); }
