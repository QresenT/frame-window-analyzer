#include "AnalyzerPopup.hpp"
#include "../Analyzer.hpp"

#include <Geode/utils/file.hpp>
#include <charconv>

using namespace geode::prelude;

namespace {
    int readInt(TextInput* input, int fallback) {
        auto text = input->getString();
        int value = fallback;
        auto first = text.data();
        auto last = first + text.size();
        auto result = std::from_chars(first, last, value);
        return result.ec == std::errc() && result.ptr == last ? value : fallback;
    }
}

namespace fwa {
    AnalyzerPopup* AnalyzerPopup::create(PauseLayer* pauseLayer) {
        auto popup = new AnalyzerPopup();
        if (popup && popup->init(470.f, 320.f) && popup->setup()) {
            popup->m_pauseLayer = pauseLayer;
            popup->autorelease();
            return popup;
        }
        CC_SAFE_DELETE(popup);
        return nullptr;
    }

    CCLabelBMFont* AnalyzerPopup::addLabel(char const* text, CCPoint position, float scale, CCTextAlignment alignment) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt", 430.f, alignment);
        label->setScale(scale);
        label->setPosition(position);
        m_mainLayer->addChild(label);
        return label;
    }

    void AnalyzerPopup::addButton(char const* text, CCPoint position, SEL_MenuHandler callback, float scale) {
        auto sprite = ButtonSprite::create(text, 0, 0, scale, false);
        auto item = CCMenuItemSpriteExtra::create(sprite, this, callback);
        item->setPosition(position);
        m_buttonMenu->addChild(item);
    }

    bool AnalyzerPopup::setup() {
        setTitle("Frame Window Analyzer", "goldFont.fnt", .75f, 18.f);
        addLabel("TPS: 240  (1 tick = 4.1667 ms)", {235.f, 276.f}, .38f);
        addButton("Record", {55.f, 244.f}, menu_selector(AnalyzerPopup::onRecord));
        addButton("Stop", {118.f, 244.f}, menu_selector(AnalyzerPopup::onStop));
        addButton("Playback", {190.f, 244.f}, menu_selector(AnalyzerPopup::onPlayback));
        addButton("Show Results", {278.f, 244.f}, menu_selector(AnalyzerPopup::onVisualize), .48f);
        m_macroLabel = addLabel("", {400.f, 244.f}, .32f);

        m_selectedInput = TextInput::create(58.f, "Input #");
        m_selectedInput->setCommonFilter(CommonFilter::Uint);
        m_selectedInput->setMaxCharCount(6);
        m_selectedInput->setPosition({48.f, 204.f});
        m_mainLayer->addChild(m_selectedInput);
        addButton("<", {92.f, 204.f}, menu_selector(AnalyzerPopup::onPrevious), .5f);
        addButton(">", {122.f, 204.f}, menu_selector(AnalyzerPopup::onNext), .5f);
        m_selectedLabel = addLabel("No input selected", {295.f, 204.f}, .38f, kCCTextAlignmentLeft);

        m_leftInput = TextInput::create(55.f, "Left");
        m_leftInput->setCommonFilter(CommonFilter::Uint);
        m_leftInput->setLabel("Search Left");
        m_leftInput->setPosition({65.f, 158.f});
        m_mainLayer->addChild(m_leftInput);
        m_rightInput = TextInput::create(55.f, "Right");
        m_rightInput->setCommonFilter(CommonFilter::Uint);
        m_rightInput->setLabel("Search Right");
        m_rightInput->setPosition({145.f, 158.f});
        m_mainLayer->addChild(m_rightInput);
        m_horizonInput = TextInput::create(70.f, "Frames");
        m_horizonInput->setCommonFilter(CommonFilter::Uint);
        m_horizonInput->setLabel("Validation");
        m_horizonInput->setPosition({210.f, 158.f});
        m_mainLayer->addChild(m_horizonInput);
        m_eventHorizonInput = TextInput::create(55.f, "Inputs");
        m_eventHorizonInput->setCommonFilter(CommonFilter::Uint);
        m_eventHorizonInput->setLabel("Next Inputs");
        m_eventHorizonInput->setPosition({285.f, 158.f});
        m_mainLayer->addChild(m_eventHorizonInput);
        m_untilEndToggle = CCMenuItemToggler::createWithStandardSprites(
            this, nullptr, .55f
        );
        m_untilEndToggle->setPosition({340.f, 158.f});
        m_buttonMenu->addChild(m_untilEndToggle);
        addLabel("Until End", {402.f, 158.f}, .31f);

        addButton("Analyze Selected", {72.f, 116.f}, menu_selector(AnalyzerPopup::onAnalyzeSelected), .42f);
        addButton("All Clicks", {178.f, 116.f}, menu_selector(AnalyzerPopup::onAnalyzeAllClicks), .42f);
        addButton("All Releases", {282.f, 116.f}, menu_selector(AnalyzerPopup::onAnalyzeAllReleases), .38f);
        addButton("All Inputs", {382.f, 116.f}, menu_selector(AnalyzerPopup::onAnalyzeAllInputs), .42f);
        addButton("Cancel", {178.f, 86.f}, menu_selector(AnalyzerPopup::onCancel), .42f);
        addButton("Export + Open", {290.f, 86.f}, menu_selector(AnalyzerPopup::onExport), .38f);
        m_statusLabel = addLabel("", {235.f, 55.f}, .34f);
        m_resultLabel = addLabel("", {235.f, 29.f}, .32f);

        auto const& settings = Analyzer::get().settings();
        m_leftInput->setString(fmt::to_string(settings.left));
        m_rightInput->setString(fmt::to_string(settings.right));
        m_horizonInput->setString(fmt::to_string(settings.validationTicks));
        m_eventHorizonInput->setString(fmt::to_string(settings.validationEvents));
        m_untilEndToggle->toggle(settings.runUntilEnd);
        refresh();
        return true;
    }

    void AnalyzerPopup::applyInputs() {
        auto& analyzer = Analyzer::get();
        auto settings = analyzer.settings();
        settings.left = readInt(m_leftInput, settings.left);
        settings.right = readInt(m_rightInput, settings.right);
        settings.validationTicks = readInt(m_horizonInput, settings.validationTicks);
        settings.validationEvents = readInt(m_eventHorizonInput, settings.validationEvents);
        settings.runUntilEnd = m_untilEndToggle->isToggled();
        analyzer.setSettings(settings);
        auto selected = readInt(m_selectedInput, static_cast<int>(analyzer.selectedInput()));
        analyzer.setSelectedInput(static_cast<std::size_t>(std::max(0, selected)));
    }

    void AnalyzerPopup::refresh() {
        auto& analyzer = Analyzer::get();
        auto const& macro = analyzer.macro();
        m_macroLabel->setString(fmt::format("Recorded Inputs: {}", macro.size()).c_str());
        m_statusLabel->setString(fmt::format("Status: {}", analyzer.statusText()).c_str());
        if (macro.empty()) {
            m_selectedInput->setString("0");
            m_selectedLabel->setString("No macro recorded");
        } else {
            auto index = analyzer.selectedInput();
            auto const& event = macro.events()[index];
            m_selectedInput->setString(fmt::to_string(index));
            m_selectedLabel->setString(fmt::format("#{}  P{} {} {}  @ {}  ({:.2f}%)", index,
                event.player, buttonName(event.button), actionName(event.pressed), event.tick, event.progress).c_str());
        }

        std::string summary = "Pause anytime to inspect progress";
        if (analyzer.results().size() > 1) {
            int bins[7] = {};
            for (auto const& result : analyzer.results()) {
                auto frames = result.primaryWindow ? result.primaryWindow->frames() : 0;
                if (frames >= 1 && frames <= 5) ++bins[frames - 1];
                else if (frames >= 6 && frames <= 10) ++bins[5];
                else if (frames >= 11) ++bins[6];
            }
            summary = fmt::format("Summary  1F:{}  2F:{}  3F:{}  4F:{}  5F:{}  6-10F:{}  11F+:{}",
                bins[0], bins[1], bins[2], bins[3], bins[4], bins[5], bins[6]);
        } else if (!analyzer.results().empty()) {
            auto const& result = analyzer.results().back();
            if (result.primaryWindow) {
                summary = fmt::format("Last: #{}  {}~{}  {}F / {:.3f} ms  ({} window{})",
                    result.inputIndex, result.primaryWindow->first, result.primaryWindow->last,
                    result.primaryWindow->frames(), result.primaryMilliseconds(), result.windows.size(),
                    result.windows.size() == 1 ? "" : "s");
            } else {
                summary = fmt::format("Last: #{}  no primary window ({} valid tick{})",
                    result.inputIndex, result.validTicks.size(), result.validTicks.size() == 1 ? "" : "s");
            }
        }
        m_resultLabel->setString(summary.c_str());
    }

    void AnalyzerPopup::closeAndResume() {
        auto pauseLayer = m_pauseLayer;
        onClose(nullptr);
        if (pauseLayer) pauseLayer->onResume(nullptr);
        else if (auto layer = PlayLayer::get()) layer->resume();
    }

    void AnalyzerPopup::onRecord(CCObject*) {
        if (auto layer = PlayLayer::get()) {
            closeAndResume();
            Loader::get()->queueInMainThread([layer] {
                if (PlayLayer::get() == layer) Analyzer::get().startRecording(layer);
            });
        }
    }
    void AnalyzerPopup::onStop(CCObject*) { Analyzer::get().stopRecording(); refresh(); }
    void AnalyzerPopup::onPlayback(CCObject*) {
        if (auto layer = PlayLayer::get()) {
            closeAndResume();
            Loader::get()->queueInMainThread([layer] {
                if (PlayLayer::get() == layer) Analyzer::get().startPlayback(layer);
            });
        }
    }
    void AnalyzerPopup::onVisualize(CCObject*) {
        if (auto layer = PlayLayer::get()) {
            closeAndResume();
            Loader::get()->queueInMainThread([layer] {
                if (PlayLayer::get() == layer) Analyzer::get().startVisualization(layer);
            });
        }
    }
    void AnalyzerPopup::onPrevious(CCObject*) {
        applyInputs();
        auto& analyzer = Analyzer::get();
        if (analyzer.selectedInput() > 0) analyzer.setSelectedInput(analyzer.selectedInput() - 1);
        refresh();
    }
    void AnalyzerPopup::onNext(CCObject*) {
        applyInputs();
        auto& analyzer = Analyzer::get();
        analyzer.setSelectedInput(analyzer.selectedInput() + 1);
        refresh();
    }
    void AnalyzerPopup::onAnalyzeSelected(CCObject*) {
        applyInputs();
        if (auto layer = PlayLayer::get()) {
            closeAndResume();
            Loader::get()->queueInMainThread([layer] {
                if (PlayLayer::get() == layer) Analyzer::get().analyzeSelected(layer);
            });
        }
    }
    void AnalyzerPopup::onAnalyzeAllClicks(CCObject*) {
        applyInputs();
        if (auto layer = PlayLayer::get()) {
            closeAndResume();
            Loader::get()->queueInMainThread([layer] {
                if (PlayLayer::get() == layer) Analyzer::get().analyzeAll(layer, InputFilter::Presses);
            });
        }
    }
    void AnalyzerPopup::onAnalyzeAllReleases(CCObject*) {
        applyInputs();
        if (auto layer = PlayLayer::get()) {
            closeAndResume();
            Loader::get()->queueInMainThread([layer] {
                if (PlayLayer::get() == layer) Analyzer::get().analyzeAll(layer, InputFilter::Releases);
            });
        }
    }
    void AnalyzerPopup::onAnalyzeAllInputs(CCObject*) {
        applyInputs();
        if (auto layer = PlayLayer::get()) {
            closeAndResume();
            Loader::get()->queueInMainThread([layer] {
                if (PlayLayer::get() == layer) Analyzer::get().analyzeAll(layer, InputFilter::Both);
            });
        }
    }
    void AnalyzerPopup::onCancel(CCObject*) { Analyzer::get().cancel(PlayLayer::get()); refresh(); }
    void AnalyzerPopup::onExport(CCObject*) {
        auto csv = Analyzer::get().exportCSV();
        auto json = Analyzer::get().exportJSON();
        if (csv && json) {
            auto opened = utils::file::openFolder(Analyzer::get().exportDirectory());
            Notification::create(opened ? "Exported files and opened folder" : "Exported files; folder could not be opened")->show();
        } else {
            Notification::create(csv ? json.unwrapErr() : csv.unwrapErr())->show();
        }
        refresh();
    }
}
