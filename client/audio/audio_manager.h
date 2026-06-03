#ifndef CLIENT_AUDIO_AUDIO_MANAGER_H
#define CLIENT_AUDIO_AUDIO_MANAGER_H

#include <SDL2/SDL_mixer.h>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void play_menu_music();
    void play_game_music();

private:
    Mix_Music* menu_music_;
    Mix_Music* game_music_;
};

#endif  // CLIENT_AUDIO_AUDIO_MANAGER_H
