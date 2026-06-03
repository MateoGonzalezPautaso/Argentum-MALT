#ifndef CLIENT_AUDIO_AUDIO_MANAGER_H
#define CLIENT_AUDIO_AUDIO_MANAGER_H

#include <string>
#include <unordered_map>

#include <SDL2/SDL_mixer.h>

#include "../config/config.h"

class AudioManager {
public:
    explicit AudioManager(const SfxConfig& sfx_cfg);
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void play_menu_music();
    void play_game_music();
    void play_sfx(const std::string& name) const;

private:
    Mix_Music* menu_music_;
    Mix_Music* game_music_;
    std::unordered_map<std::string, Mix_Chunk*> sfx_;
};

#endif  // CLIENT_AUDIO_AUDIO_MANAGER_H
