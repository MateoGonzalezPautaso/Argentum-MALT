#ifndef CLIENT_CREDENTIAL_FORM_H
#define CLIENT_CREDENTIAL_FORM_H

#include <string>

#include <SDL2/SDL.h>

#include "../input/chat_input.h"

// Encapsula el par de campos username/password con manejo de foco y TAB.
class CredentialForm {
public:
    CredentialForm();

    // Procesa TAB y despacha teclas al campo activo.
    // El caller es responsable de notificar clicks en campos via
    // focus_first() / focus_second() y clicks en el resto via blur().
    bool handle_keydown(const SDL_Event& event);

    // Despacha el evento al campo activo (para SDL_KEYDOWN y SDL_TEXTINPUT).
    void dispatch_to_active(const SDL_Event& event);

    // Manejo de foco explícito (llamado por el controller al detectar clicks).
    void focus_first();
    void focus_second();
    void blur();

    void reset();

    const std::string& first_value() const;
    const std::string& second_value() const;

    const ChatInput& first_field() const { return first_field_; }
    const ChatInput& second_field() const { return second_field_; }

private:
    ChatInput first_field_;
    ChatInput second_field_;
    int active_field_ = 0;
};

#endif  // CLIENT_CREDENTIAL_FORM_H
