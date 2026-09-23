#pragma once

#include <Geode/Geode.hpp>
#include <cstdint>
#include <vector>

namespace fwa {
    inline constexpr int TPS = 240;
    inline constexpr int RAW_TICKS_PER_FRAME = 2;
    inline constexpr int RAW_TPS = TPS * RAW_TICKS_PER_FRAME;

    struct InputEvent {
        std::int64_t tick = 0;
        std::uint8_t player = 1;
        PlayerButton button = PlayerButton::Jump;
        bool pressed = false;
        float progress = 0.f;
    };

    class Macro {
    public:
        void clear();
        void add(InputEvent event);
        std::vector<InputEvent> const& events() const;
        bool empty() const;
        std::size_t size() const;

    private:
        std::vector<InputEvent> m_events;
    };

    char const* buttonName(PlayerButton button);
    char const* actionName(bool pressed);
}
