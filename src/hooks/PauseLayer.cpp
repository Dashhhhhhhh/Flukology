#include "PauseLayer.hpp"

using namespace geode::prelude;

void HookPauseLayer::customSetup() {
    PauseLayer::customSetup();

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto menu = CCMenu::create();
    menu->setID("dashcoach-pause-menu"_spr);
    menu->setPosition(winSize.width / 2.f, 72.f);

    auto buttonSprite = ButtonSprite::create("DashCoach");
    auto button = CCMenuItemExt::createSpriteExtra(buttonSprite, [](CCObject*) {
        // Placeholder for future DashCoach pause-menu actions.
    });

    button->setID("dashcoach-button"_spr);
    menu->addChild(button);
    this->addChild(menu);
}
