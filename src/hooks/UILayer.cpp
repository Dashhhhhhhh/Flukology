#include "UILayer.hpp"

#include "../ModManager.hpp"
#include "PlayLayer.hpp"

using namespace geode::prelude;

bool HookUILayer::init(GJBaseGameLayer* baseGame) {
    if (!UILayer::init(baseGame)) {
        return false;
    }

    if (auto* playLayer = typeinfo_cast<PlayLayer*>(baseGame)) {
        auto* director = CCDirector::sharedDirector();
        auto winSize = director->getWinSize();
        auto fields = m_fields.self();
        auto* manager = ModManager::sharedState();

        fields->m_switcherMenu = CCMenu::create();
        fields->m_switcherMenu->setPosition(ccp(winSize.width / 2, director->getScreenBottom() + 17.f));
        fields->m_switcherMenu->setID("startpos-switcher-menu"_spr);

        fields->m_switcherLabel = CCLabelBMFont::create("0/0", "bigFont.fnt");
        fields->m_switcherLabel->setID("startpos-index-label"_spr);
        fields->m_switcherLabel->setScale(0.6f);

        auto* nextSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        auto* prevSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        nextSprite->setScale(0.6f);
        nextSprite->setFlipX(true);
        prevSprite->setScale(0.6f);

        fields->m_nextSwitcherBtn = CCMenuItemExt::createSpriteExtra(nextSprite, [this](CCObject*) {
            auto* layer = static_cast<HookPlayLayer*>(PlayLayer::get());
            if (layer->isLearnModeEnabled()) {
                return;
            }
            layer->updateStartPos(layer->m_fields->m_startPosIdx + 1);
        });
        fields->m_nextSwitcherBtn->setID("startpos-next-button"_spr);

        fields->m_prevSwitcherBtn = CCMenuItemExt::createSpriteExtra(prevSprite, [this](CCObject*) {
            auto* layer = static_cast<HookPlayLayer*>(PlayLayer::get());
            if (layer->isLearnModeEnabled()) {
                return;
            }
            layer->updateStartPos(layer->m_fields->m_startPosIdx - 1);
        });
        fields->m_prevSwitcherBtn->setID("startpos-prev-button"_spr);

        fields->m_switcherMenu->addChild(fields->m_prevSwitcherBtn);
        fields->m_switcherMenu->addChild(fields->m_switcherLabel);
        fields->m_switcherMenu->addChild(fields->m_nextSwitcherBtn);
        fields->m_switcherMenu->setLayout(AxisLayout::create()->setAutoScale(false)->setGap(10.f));

        if (manager->m_hideBtns) {
            fields->m_prevSwitcherBtn->setVisible(false);
            fields->m_nextSwitcherBtn->setVisible(false);
        }

        addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "leftSwitch"), [](const Keybind&, bool down, bool repeat, double) {
            if (down && !repeat) {
                auto* layer = static_cast<HookPlayLayer*>(PlayLayer::get());
                if (layer->isLearnModeEnabled()) {
                    return;
                }
                layer->updateStartPos(layer->m_fields->m_startPosIdx - 1);
            }
        });

        addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "rightSwitch"), [](const Keybind&, bool down, bool repeat, double) {
            if (down && !repeat) {
                auto* layer = static_cast<HookPlayLayer*>(PlayLayer::get());
                if (layer->isLearnModeEnabled()) {
                    return;
                }
                layer->updateStartPos(layer->m_fields->m_startPosIdx + 1);
            }
        });

        this->addChild(fields->m_switcherMenu);
    }

    return true;
}

void HookUILayer::updateUI() {
    auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
    auto fields = m_fields.self();
    auto* manager = ModManager::sharedState();

    if (!fields->m_switcherMenu || !fields->m_switcherLabel) {
        return;
    }

    if (playLayer->m_fields->m_startPosObjects.empty()) {
        fields->m_switcherMenu->setVisible(false);
        return;
    }

    fields->m_switcherMenu->setVisible(true);
    fields->m_switcherLabel->setString(fmt::format("{}/{}", playLayer->m_fields->m_startPosIdx, playLayer->m_fields->m_startPosObjects.size()).c_str());
    fields->m_switcherLabel->limitLabelWidth(40.f, 0.6f, 0.f);
    auto hideSwitchButtons = manager->m_hideBtns || playLayer->isLearnModeEnabled();
    fields->m_prevSwitcherBtn->setVisible(!hideSwitchButtons);
    fields->m_nextSwitcherBtn->setVisible(!hideSwitchButtons);

    fields->m_switcherMenu->stopActionByTag(676767677);
    if (fields->m_firstUpdate && manager->m_dontFadeOnStart) {
        fields->m_switcherMenu->setOpacity(manager->m_opacity);
        fields->m_firstUpdate = false;
    }
    else {
        auto* action = CCSequence::create(
            CCEaseInOut::create(CCFadeTo::create(0.3f, 255), 2.f),
            CCDelayTime::create(0.5f),
            CCEaseInOut::create(CCFadeTo::create(0.5f, manager->m_opacity), 2.f),
            nullptr
        );
        action->setTag(676767677);
        fields->m_switcherMenu->runAction(action);
        fields->m_firstUpdate = false;
    }
}
