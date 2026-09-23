#include "AnalysisResult.hpp"
#include <algorithm>

namespace fwa {
    void AnalysisResult::finalize() {
        windows.clear();
        primaryWindow.reset();
        std::sort(validTicks.begin(), validTicks.end());
        validTicks.erase(std::unique(validTicks.begin(), validTicks.end()), validTicks.end());
        for (auto tick : validTicks) {
            if (windows.empty() || tick != windows.back().last + RAW_TICKS_PER_FRAME) {
                windows.push_back({tick, tick});
            } else {
                windows.back().last = tick;
            }
        }
        for (auto const& window : windows) {
            if (input.tick >= window.first && input.tick <= window.last) {
                primaryWindow = window;
                break;
            }
        }
    }

    double AnalysisResult::primaryMilliseconds() const {
        return primaryWindow ? static_cast<double>(primaryWindow->frames()) * 1000.0 / TPS : 0.0;
    }
}
