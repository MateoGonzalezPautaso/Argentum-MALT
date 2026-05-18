#ifndef CHAT_INPUT_H
#define CHAT_INPUT_H

#include <string>

#include <SDL2/SDL.h>

class ChatInput {
private:
    std::string text;
    bool focused = false;

public:
    bool handle_text_input(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
    void set_focus(bool value);
    bool is_focused() const;
    const std::string& get_text() const;
};

#endif  // CHAT_INPUT_H
