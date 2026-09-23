#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "Analyzer.hpp"
#include "ui/AnalyzerPopup.hpp"
#include "ui/ResultOverlay.hpp"

using namespace geode::prelude;

namespace {
    constexpr int HUD_TAG = 0x465741;

    PlayLayer* asPlayLayer(GJBaseGameLayer* base) {
        auto play = PlayLayer::get();
        return play && static_cast<GJBaseGameLayer*>(play) == base ? play : nullptr;
    }
}

class $modify(FWABaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        fwa::Analyzer::get().onButton(this, down, button, isPlayer1);
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        fwa::Analyzer::get().beforeProcessCommands(this);
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        fwa::Analyzer::get().afterProcessCommands(this);
    }

    void update(float dt) {
        GJBaseGameLayer::update(dt);
        if (auto play = asPlayLayer(this)) {
            fwa::Analyzer::get().afterUpdate(play);
            if (auto label = typeinfo_cast<CCLabelBMFont*>(play->getChildByTag(HUD_TAG))) {
                auto const mode = fwa::Analyzer::get().mode();
                label->setVisible(mode != fwa::Mode::Idle);
                if (mode != fwa::Mode::Idle) label->setString(fwa::Analyzer::get().statusText().c_str());
            }
        }
    }
};

class $modify(FWAPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        fwa::Analyzer::get().onLayerInit(this);
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setTag(HUD_TAG);
        label->setScale(.35f);
        label->setAnchorPoint({0.f, 1.f});
        auto win = CCDirector::sharedDirector()->getWinSize();
        label->setPosition({8.f, win.height - 8.f});
        label->setZOrder(1000);
        label->setVisible(false);
        addChild(label);
        auto resultOverlay = fwa::ResultOverlay::create();
        resultOverlay->setZOrder(1001);
        addChild(resultOverlay);

        if (Loader::get()->isModLoaded("syzzi.click_between_frames") ||
            Loader::get()->isModLoaded("eclipse.eclipse-menu") ||
            Loader::get()->isModLoaded("thesillydoggo.qolmod") ||
            Loader::get()->isModLoaded("tobyadd.gdh")) {
            log::warn("A mod capable of altering input/TPS semantics is loaded. Disable CBF, TPS bypass, and speedhack before trusting 240 TPS results.");
        }
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        fwa::Analyzer::get().onReset(this);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        fwa::Analyzer::get().onDeath(this, player, object);
    }

    void levelComplete() {
        if (!fwa::Analyzer::get().onLevelComplete(this)) PlayLayer::levelComplete();
    }

    void onQuit() {
        fwa::Analyzer::get().onLayerExit(this);
        PlayLayer::onQuit();
    }
};

class $modify(FWAPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = CCMenu::create();
        auto button = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Frame Window Analyzer", 0, 0, .45f, false),
            this,
            menu_selector(FWAPauseLayer::onOpenAnalyzer)
        );
        menu->addChild(button);
        auto win = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition({win.width / 2.f, 28.f});
        menu->setZOrder(100);
        addChild(menu);
    }

    void onOpenAnalyzer(CCObject*) {
        fwa::AnalyzerPopup::create(this)->show();
    }
};
