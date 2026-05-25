#include "chat_history.h"

#include <algorithm>

#include <SDL2/SDL.h>

void ChatHistory::add_message(ChatMsgType type, const std::string& sender,
                              const std::string& text) {
    messages.push_back(
            {type, sender, text, static_cast<uint32_t>(SDL_GetTicks())});

    if (messages.size() > MAX_MESSAGES) {
        messages.erase(messages.begin(),
                       messages.begin() + static_cast<long>(messages.size() - MAX_MESSAGES));
    }
}

const std::vector<ChatMessage>& ChatHistory::get_messages() const { return messages; }

void ChatHistory::clear() { messages.clear(); }
