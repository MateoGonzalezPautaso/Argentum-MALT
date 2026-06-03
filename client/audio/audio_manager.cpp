#include "audio_manager.h"

#include <iostream>
#include <string>

AudioManager::AudioManager(): menu_music_(nullptr), game_music_(nullptr) {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "AudioManager: Mix_OpenAudio failed - " << Mix_GetError() << std::endl;
        return;
    }

    menu_music_ = Mix_LoadMUS("assets/midi/1.MID");
    if (!menu_music_) {
        std::cerr << "AudioManager: failed to load menu music - " << Mix_GetError()
                  << std::endl;
    }

    game_music_ = Mix_LoadMUS("assets/midi/5.mid");
    if (!game_music_) {
        std::cerr << "AudioManager: failed to load game music - " << Mix_GetError()
                  << std::endl;
    }
}

AudioManager::~AudioManager() {
    Mix_HaltMusic();
    if (menu_music_) {
        Mix_FreeMusic(menu_music_);
    }
    if (game_music_) {
        Mix_FreeMusic(game_music_);
    }
    Mix_CloseAudio();
}

void AudioManager::play_menu_music() {
    if (!menu_music_) {
        return;
    }
    Mix_VolumeMusic(MIX_MAX_VOLUME);
    Mix_PlayMusic(menu_music_, -1);
}

void AudioManager::play_game_music() {
    if (!game_music_) {
        return;
    }
    int volume = static_cast<int>(MIX_MAX_VOLUME * 0.3f);
    Mix_VolumeMusic(volume);
    Mix_PlayMusic(game_music_, -1);
}
