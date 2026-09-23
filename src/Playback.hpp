#pragma once

#include "Macro.hpp"
#include <optional>
#include <utility>

namespace fwa {
    class Playback {
    public:
        void load(Macro const& macro, std::optional<std::size_t> movedInput = std::nullopt, int offset = 0);
        void reset();
        std::vector<InputEvent> takeAt(std::int64_t tick);
        bool exhausted() const;

    private:
        std::vector<std::pair<std::size_t, InputEvent>> m_schedule;
        std::size_t m_cursor = 0;
    };
}
