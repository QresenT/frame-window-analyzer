#pragma once

#include "../AnalysisResult.hpp"
#include <Geode/Geode.hpp>
#include <deque>

namespace fwa {
    inline constexpr int RESULT_OVERLAY_TAG = 0x465742;

    class ResultOverlay : public cocos2d::CCNode {
    public:
        static ResultOverlay* create();
        void showInput(std::size_t index, InputEvent const& event, AnalysisResult const* result);
        void clearHistory();

    private:
        cocos2d::CCLayerColor* m_background = nullptr;
        cocos2d::CCLabelBMFont* m_frameLabel = nullptr;
        cocos2d::CCLabelBMFont* m_detailLabel = nullptr;
        cocos2d::CCLabelBMFont* m_historyLabel = nullptr;
        std::deque<std::string> m_history;

        bool initOverlay();
        void hideBadge();
    };
}
