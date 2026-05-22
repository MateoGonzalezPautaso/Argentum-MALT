#ifndef CLIENT_ANIMATION_SYSTEM_H
#define CLIENT_ANIMATION_SYSTEM_H

#include <cstdint>

class AnimationSystem {
public:
    AnimationSystem() = default;

    void set_now(uint32_t now) { now_ = now; }

    template <typename T>
    bool tick(T& animable) {
        if (!animable.animated || animable.frame_ms == 0)
            return false;
        if (now_ - animable.last_ticks < animable.frame_ms)
            return false;
        animable.last_ticks = now_;
        animable.current_frame = (animable.current_frame + 1) % animable.frames.size();
        return true;
    }

private:
    uint32_t now_ = 0;
};

#endif
