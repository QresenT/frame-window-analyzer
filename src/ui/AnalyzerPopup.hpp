#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

namespace fwa {
    class AnalyzerPopup : public geode::Popup {
    protected:
        PauseLayer* m_pauseLayer = nullptr;
        geode::TextInput* m_selectedInput = nullptr;
        geode::TextInput* m_leftInput = nullptr;
        geode::TextInput* m_rightInput = nullptr;
        geode::TextInput* m_horizonInput = nullptr;
        geode::TextInput* m_eventHorizonInput = nullptr;
        CCMenuItemToggler* m_untilEndToggle = nullptr;
        cocos2d::CCLabelBMFont* m_macroLabel = nullptr;
        cocos2d::CCLabelBMFont* m_selectedLabel = nullptr;
        cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
        cocos2d::CCLabelBMFont* m_resultLabel = nullptr;

        bool setup();
        void refresh();
        void applyInputs();
        void closeAndResume();
        void addButton(char const* text, cocos2d::CCPoint position, cocos2d::SEL_MenuHandler callback, float scale = .55f);
        cocos2d::CCLabelBMFont* addLabel(char const* text, cocos2d::CCPoint position, float scale, cocos2d::CCTextAlignment alignment = cocos2d::kCCTextAlignmentCenter);

        void onRecord(cocos2d::CCObject*);
        void onStop(cocos2d::CCObject*);
        void onPlayback(cocos2d::CCObject*);
        void onVisualize(cocos2d::CCObject*);
        void onPrevious(cocos2d::CCObject*);
        void onNext(cocos2d::CCObject*);
        void onAnalyzeSelected(cocos2d::CCObject*);
        void onAnalyzeAllClicks(cocos2d::CCObject*);
        void onAnalyzeAllReleases(cocos2d::CCObject*);
        void onAnalyzeAllInputs(cocos2d::CCObject*);
        void onCancel(cocos2d::CCObject*);
        void onExport(cocos2d::CCObject*);

    public:
        static AnalyzerPopup* create(PauseLayer* pauseLayer);
    };
}
