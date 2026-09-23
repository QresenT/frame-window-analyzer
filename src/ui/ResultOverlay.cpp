#include "ResultOverlay.hpp"

using namespace geode::prelude;

namespace fwa {
    ResultOverlay* ResultOverlay::create() {
        auto node = new ResultOverlay();
        if (node && node->init() && node->initOverlay()) {
            node->autorelease();
            return node;
        }
        CC_SAFE_DELETE(node);
        return nullptr;
    }

    bool ResultOverlay::initOverlay() {
        auto win = CCDirector::sharedDirector()->getWinSize();
        setContentSize(win);
        setTag(RESULT_OVERLAY_TAG);

        m_background = CCLayerColor::create({45, 45, 45, 220}, 190.f, 54.f);
        m_background->setPosition({win.width / 2.f - 95.f, win.height - 72.f});
        addChild(m_background);

        m_frameLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_frameLabel->setPosition({win.width / 2.f, win.height - 38.f});
        m_frameLabel->setScale(.72f);
        addChild(m_frameLabel);

        m_detailLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_detailLabel->setPosition({win.width / 2.f, win.height - 61.f});
        m_detailLabel->setScale(.32f);
        addChild(m_detailLabel);

        m_historyLabel = CCLabelBMFont::create("", "chatFont.fnt", 170.f, kCCTextAlignmentRight);
        m_historyLabel->setAnchorPoint({1.f, 1.f});
        m_historyLabel->setPosition({win.width - 10.f, win.height - 14.f});
        m_historyLabel->setScale(.45f);
        addChild(m_historyLabel);
        hideBadge();
        return true;
    }

    void ResultOverlay::clearHistory() {
        m_history.clear();
        m_historyLabel->setString("");
        hideBadge();
    }

    void ResultOverlay::hideBadge() {
        m_background->setVisible(false);
        m_frameLabel->setVisible(false);
        m_detailLabel->setVisible(false);
    }

    void ResultOverlay::showInput(std::size_t index, InputEvent const& event, AnalysisResult const* result) {
        std::string framesText = "NO WINDOW";
        ccColor3B color = {145, 145, 145};
        std::int64_t frames = 0;
        if (result && result->primaryWindow) {
            frames = result->primaryWindow->frames();
            framesText = fmt::format("{}F", frames);
            if (frames <= 2) color = {255, 75, 75};
            else if (frames <= 5) color = {255, 165, 55};
            else if (frames <= 10) color = {80, 230, 105};
            else color = {65, 220, 235};
        }

        m_background->setColor(color);
        m_background->setOpacity(215);
        m_frameLabel->setString(framesText.c_str());
        m_detailLabel->setString(fmt::format("#{}  P{} {} {}  @ {}", index, event.player,
            buttonName(event.button), actionName(event.pressed), event.tick).c_str());
        m_background->setVisible(true);
        m_frameLabel->setVisible(true);
        m_detailLabel->setVisible(true);

        m_history.push_front(fmt::format("#{}  {:>9}  {}", index, framesText, actionName(event.pressed)));
        while (m_history.size() > 6) m_history.pop_back();
        std::string history;
        for (auto const& line : m_history) history += line + "\n";
        m_historyLabel->setString(history.c_str());

        stopAllActions();
        runAction(CCSequence::create(
            CCDelayTime::create(.7f),
            CCCallFunc::create(this, callfunc_selector(ResultOverlay::hideBadge)),
            nullptr
        ));
    }
}
