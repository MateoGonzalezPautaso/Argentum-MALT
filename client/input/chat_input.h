#ifndef CHAT_INPUT_H
#define CHAT_INPUT_H

#include <string>

#include <SDL2/SDL.h>

class ChatInput {
private:
    std::string text;
    std::string pending_message;
    bool focused = false;

public:
    bool handle_text_input(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
    bool consume_event(const SDL_Event& event);
    bool set_focus(bool value);
    bool is_focused() const;
    const std::string& get_text() const;
    bool has_pending_message() const;
    std::string pop_pending_message();
    void clear();
};

#endif  // CHAT_INPUT_H
