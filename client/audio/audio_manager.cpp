#include "audio_manager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

AudioManager::AudioManager(const SfxConfig& sfx_cfg): menu_music_(nullptr), game_music_(nullptr) {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "AudioManager: Mix_OpenAudio failed - " << Mix_GetError() << std::endl;
        return;
    }

    menu_music_ = Mix_LoadMUS("assets/midi/1.MID");
    if (!menu_music_) {
        std::cerr << "AudioManager: failed to load menu music - " << Mix_GetError() << std::endl;
    }

    game_music_ = Mix_LoadMUS("assets/Mp3/31.mp3");
    if (!game_music_) {
        std::cerr << "AudioManager: failed to load game music - " << Mix_GetError() << std::endl;
    }

    for (const auto& [name, filename]: sfx_cfg.sounds) {
        std::string full_path = "assets/SoundsOgg/" + filename;
        Mix_Chunk* chunk = Mix_LoadWAV(full_path.c_str());
        if (!chunk) {
            std::cerr << "AudioManager: failed to load sfx '" << name << "' from " << full_path
                      << " - " << Mix_GetError() << std::endl;
            continue;
        }
        sfx_[name] = chunk;
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
    for (const auto& [name, chunk]: sfx_) {
        Mix_FreeChunk(chunk);
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

void AudioManager::play_sfx(const std::string& name) const {
    auto it = sfx_.find(name);
    if (it == sfx_.end()) {
        return;
    }
    Mix_PlayChannel(-1, it->second, 0);
}

void AudioManager::set_player_position(int x, int y) {
    player_x_ = x;
    player_y_ = y;
}

int AudioManager::distance_volume(int source_x, int source_y) const {
    int dx = source_x - player_x_;
    int dy = source_y - player_y_;
    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
    float clamped = std::min(dist, static_cast<float>(MAX_HEAR_RANGE));
    return static_cast<int>(MIX_MAX_VOLUME * (1.0f - clamped / MAX_HEAR_RANGE));
}

void AudioManager::play_sfx_at(const std::string& name, int source_x, int source_y) const {
    auto it = sfx_.find(name);
    if (it == sfx_.end()) {
        return;
    }
    int channel = Mix_PlayChannel(-1, it->second, 0);
    if (channel >= 0) {
        Mix_Volume(channel, distance_volume(source_x, source_y));
    }
}
