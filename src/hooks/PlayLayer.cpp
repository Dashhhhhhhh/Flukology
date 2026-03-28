#include "PlayLayer.hpp"

#include "../ModManager.hpp"
#include "UILayer.hpp"

#include <Geode/binding/StartPosObject.hpp>

#include <algorithm>

using namespace geode::prelude;

void HookPlayLayer::addObject(GameObject* obj) {
    if (obj->m_objectID == 31) {
        auto startPos = static_cast<StartPosObject*>(obj);
        if (!startPos->m_startSettings->m_disableStartPos || !ModManager::sharedState()->m_ignoreDisabled) {
            m_fields->m_startPosObjects.push_back(obj);
        }
    }

    PlayLayer::addObject(obj);
}

void HookPlayLayer::updateStartPos(int idx) {
    auto fields = m_fields.self();

    if (fields->m_startPosObjects.empty()) {
        return;
    }

    if (idx < 0) {
        idx = static_cast<int>(fields->m_startPosObjects.size());
    }
    if (idx > static_cast<int>(fields->m_startPosObjects.size())) {
        idx = 0;
    }

    m_isTestMode = idx != 0;
    updateTestModeLabel();

    m_currentCheckpoint = nullptr;
    fields->m_startPosIdx = idx;

    auto* object = idx > 0 ? fields->m_startPosObjects[idx - 1].data() : nullptr;
    setStartPosObject(static_cast<StartPosObject*>(object));

    if (m_isPracticeMode) {
        resetLevelFromStart();
    }

    resetLevel();
    startMusic();

    if (auto* uiLayer = typeinfo_cast<UILayer*>(m_uiLayer)) {
        static_cast<HookUILayer*>(uiLayer)->updateUI();
    }
}

void HookPlayLayer::createObjectsFromSetupFinished() {
    PlayLayer::createObjectsFromSetupFinished();

    auto fields = m_fields.self();
    std::sort(fields->m_startPosObjects.begin(), fields->m_startPosObjects.end(), [](auto const& a, auto const& b) {
        return a->getPositionX() < b->getPositionX();
    });

    if (m_startPosObject) {
        auto it = std::find(fields->m_startPosObjects.begin(), fields->m_startPosObjects.end(), m_startPosObject);
        if (it != fields->m_startPosObjects.end()) {
            fields->m_startPosIdx = static_cast<int>(std::distance(fields->m_startPosObjects.begin(), it)) + 1;
        }
    }

    if (auto* uiLayer = typeinfo_cast<UILayer*>(m_uiLayer)) {
        static_cast<HookUILayer*>(uiLayer)->updateUI();
    }
}
