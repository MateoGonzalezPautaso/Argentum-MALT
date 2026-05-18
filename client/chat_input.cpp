#include "chat_input.h"

bool ChatInput::handle_text_input(const SDL_Event& event) {
    if (!focused) {
        return false;
    }
    text += event.text.text;
    return true;
}

bool ChatInput::handle_keydown(const SDL_Event& event) {
    if (!focused) {
        return false;
    }

    switch (event.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            // todo aca habria que enviar el mensaje al servidor antes de limpiar el input
            text.clear();
            return true;
        case SDLK_BACKSPACE:
            if (!text.empty()) {
                text.pop_back();
            }
            return true;
        case SDLK_ESCAPE:
            focused = false;
            return true;
        default:
            // mientras el chat esta activo, bloquea acciones de gameplay.
            return true;
    }
}

void ChatInput::set_focus(bool value) { focused = value; }

bool ChatInput::is_focused() const { return focused; }

const std::string& ChatInput::get_text() const { return text; }
