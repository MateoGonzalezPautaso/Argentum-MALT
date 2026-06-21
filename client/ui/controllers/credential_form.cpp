#include "credential_form.h"

CredentialForm::CredentialForm() { first_field_.set_focus(true); }

bool CredentialForm::handle_keydown(const SDL_Event& event) {
    if (event.type != SDL_KEYDOWN)
        return false;

    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)
        return true;

    if (event.key.keysym.sym == SDLK_TAB) {
        active_field_ = (active_field_ + 1) % 2;
        first_field_.set_focus(active_field_ == 0);
        second_field_.set_focus(active_field_ == 1);
    }

    return false;
}

void CredentialForm::dispatch_to_active(const SDL_Event& event) {
    if (active_field_ == 0)
        first_field_.consume_event(event);
    else
        second_field_.consume_event(event);
}

void CredentialForm::focus_first() {
    active_field_ = 0;
    first_field_.set_focus(true);
    second_field_.set_focus(false);
}

void CredentialForm::focus_second() {
    active_field_ = 1;
    first_field_.set_focus(false);
    second_field_.set_focus(true);
}

void CredentialForm::blur() {
    first_field_.set_focus(false);
    second_field_.set_focus(false);
}

void CredentialForm::reset() {
    first_field_.clear();
    second_field_.clear();
}

const std::string& CredentialForm::first_value() const { return first_field_.get_text(); }

const std::string& CredentialForm::second_value() const { return second_field_.get_text(); }
