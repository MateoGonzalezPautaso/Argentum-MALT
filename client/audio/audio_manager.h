#ifndef CLIENT_AUDIO_AUDIO_MANAGER_H
#define CLIENT_AUDIO_AUDIO_MANAGER_H

#include <string>
#include <unordered_map>

#include <SDL2/SDL_mixer.h>

#include "../config/config.h"

class AudioManager {
public:
    explicit AudioManager(const AudioConfig& audio_cfg);
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void play_menu_music();
    void play_game_music();
    void play_sfx(const std::string& name) const;
    void play_sfx_at(const std::string& name, int source_x, int source_y) const;
    void set_player_position(int x, int y);
    void set_muted(bool muted);
    bool is_muted() const { return muted_; }

private:
    int player_x_ = 0;
    int player_y_ = 0;
    bool muted_ = false;
    int current_music_volume_ = MIX_MAX_VOLUME;

    static constexpr int MAX_HEAR_RANGE = 640;

    int distance_volume(int source_x, int source_y) const;

    Mix_Music* menu_music_;
    Mix_Music* game_music_;
    std::unordered_map<std::string, Mix_Chunk*> sfx_;
};

#endif  // CLIENT_AUDIO_AUDIO_MANAGER_H
