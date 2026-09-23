#pragma once

#include "Macro.hpp"
#include <optional>
#include <vector>

namespace fwa {
    struct TickWindow {
        std::int64_t first = 0;
        std::int64_t last = 0;

        std::int64_t frames() const { return (last - first) / RAW_TICKS_PER_FRAME + 1; }
    };

    struct AnalysisResult {
        std::size_t inputIndex = 0;
        InputEvent input;
        std::vector<std::int64_t> validTicks;
        std::vector<TickWindow> windows;
        std::optional<TickWindow> primaryWindow;

        void finalize();
        double primaryMilliseconds() const;
    };
}
