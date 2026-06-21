#include "audio_manager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

AudioManager::AudioManager(const AudioConfig& audio_cfg): menu_music_(nullptr), game_music_(nullptr) {
    if (Mix_OpenAudio(audio_cfg.frequency, MIX_DEFAULT_FORMAT, audio_cfg.channels,
                      audio_cfg.chunksize) < 0) {
        std::cerr << "AudioManager: Mix_OpenAudio failed - " << Mix_GetError() << std::endl;
        return;
    }

    menu_music_ = Mix_LoadMUS(audio_cfg.midi_music_path.c_str());
    if (!menu_music_) {
        std::cerr << "AudioManager: failed to load menu music - " << Mix_GetError() << std::endl;
    }

    game_music_ = Mix_LoadMUS(audio_cfg.mp3_music_path.c_str());
    if (!game_music_) {
        std::cerr << "AudioManager: failed to load game music - " << Mix_GetError() << std::endl;
    }

    for (const auto& [name, filename]: audio_cfg.sfx.sounds) {
        std::string full_path = audio_cfg.sfx_prefix + filename;
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
    if (!menu_music_)
        return;
    current_music_volume_ = MIX_MAX_VOLUME;
    Mix_VolumeMusic(muted_ ? 0 : current_music_volume_);
    Mix_PlayMusic(menu_music_, -1);
}

void AudioManager::play_game_music() {
    if (!game_music_)
        return;
    current_music_volume_ = static_cast<int>(MIX_MAX_VOLUME * 0.3f);
    Mix_VolumeMusic(muted_ ? 0 : current_music_volume_);
    Mix_PlayMusic(game_music_, -1);
}

void AudioManager::set_muted(bool muted) {
    muted_ = muted;
    if (muted) {
        Mix_VolumeMusic(0);
        Mix_Volume(-1, 0);
    } else {
        Mix_VolumeMusic(current_music_volume_);
        Mix_Volume(-1, MIX_MAX_VOLUME);
    }
}

void AudioManager::play_sfx(const std::string& name) const {
    if (muted_)
        return;
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
    if (muted_)
        return;
    auto it = sfx_.find(name);
    if (it == sfx_.end()) {
        return;
    }
    int channel = Mix_PlayChannel(-1, it->second, 0);
    if (channel >= 0) {
        Mix_Volume(channel, distance_volume(source_x, source_y));
    }
}
