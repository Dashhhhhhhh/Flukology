#include "PlayLayer.hpp"

#include "../ModManager.hpp"
#include "UILayer.hpp"

#include <Geode/binding/StartPosObject.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    constexpr float kMinRunLength = 0.01f;

    float clampPercent(float percent) {
        return std::clamp(percent, 0.f, 100.f);
    }

    int roundedPercent(float percent) {
        return static_cast<int>(std::lround(clampPercent(percent)));
    }
}

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

    resetActiveRunTracking();
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

    rebuildRunStats();
    resetActiveRunTracking();

    if (auto* uiLayer = typeinfo_cast<UILayer*>(m_uiLayer)) {
        static_cast<HookUILayer*>(uiLayer)->updateUI();
    }
}

void HookPlayLayer::resetLevel() {
    PlayLayer::resetLevel();
    resetActiveRunTracking();
}

void HookPlayLayer::postUpdate(float dt) {
    PlayLayer::postUpdate(dt);

    auto fields = m_fields.self();
    if (fields->m_runStats.empty() || !isGameplayActive()) {
        return;
    }

    auto currentPercent = clampPercent(getCurrentPercent());
    auto runIndex = findRunIndexForPercent(currentPercent);
    markRunAttempt(runIndex);
    recordBestReach(runIndex, currentPercent);
}

void HookPlayLayer::destroyPlayer(PlayerObject* player, GameObject* object) {
    if (player == m_player1) {
        auto fields = m_fields.self();
        if (!fields->m_runStats.empty()) {
            auto deathPercent = clampPercent(getCurrentPercent());
            auto runIndex = findRunIndexForPercent(deathPercent);
            markRunAttempt(runIndex);
            recordBestReach(runIndex, deathPercent);
            fields->m_runStats[runIndex].m_deathPercents.push_back(roundedPercent(deathPercent));
            resetActiveRunTracking();
        }
    }

    PlayLayer::destroyPlayer(player, object);
}

void HookPlayLayer::levelComplete() {
    auto fields = m_fields.self();
    if (!fields->m_runStats.empty()) {
        auto finalRunIndex = static_cast<int>(fields->m_runStats.size()) - 1;
        markRunAttempt(finalRunIndex);
        recordBestReach(finalRunIndex, 100.f);
        fields->m_runStats[finalRunIndex].m_clears += 1;
        resetActiveRunTracking();
    }

    PlayLayer::levelComplete();
}

std::vector<RunStats> const& HookPlayLayer::getRunStats() {
    return m_fields->m_runStats;
}

int HookPlayLayer::findRunIndexForPercent(float percent) {
    auto fields = m_fields.self();
    if (fields->m_runStats.empty()) {
        return -1;
    }

    auto clampedPercent = clampPercent(percent);
    for (auto index = static_cast<int>(fields->m_runStats.size()) - 1; index >= 0; --index) {
        if (clampedPercent + 0.001f >= fields->m_runStats[index].m_startPercent) {
            return index;
        }
    }

    return 0;
}

void HookPlayLayer::rebuildRunStats() {
    auto fields = m_fields.self();
    fields->m_runStats.clear();

    std::vector<float> boundaries;
    boundaries.reserve(fields->m_startPosObjects.size() + 2);
    boundaries.push_back(0.f);

    auto levelLength = std::max(m_levelLength, 1.f);
    for (auto const& object : fields->m_startPosObjects) {
        auto boundary = clampPercent(object->getPositionX() / levelLength * 100.f);
        if (boundary >= 100.f - kMinRunLength) {
            continue;
        }

        if (std::abs(boundary - boundaries.back()) >= kMinRunLength) {
            boundaries.push_back(boundary);
        }
    }

    boundaries.push_back(100.f);

    for (size_t index = 0; index + 1 < boundaries.size(); ++index) {
        auto startPercent = boundaries[index];
        auto endPercent = boundaries[index + 1];
        if (endPercent - startPercent < kMinRunLength) {
            continue;
        }

        RunStats stats;
        stats.m_startPercent = startPercent;
        stats.m_endPercent = endPercent;
        stats.m_bestReach = startPercent;
        fields->m_runStats.push_back(std::move(stats));
    }

    if (fields->m_runStats.empty()) {
        fields->m_runStats.push_back(RunStats {});
    }
}

void HookPlayLayer::resetActiveRunTracking() {
    m_fields->m_activeRunIdx = -1;
}

void HookPlayLayer::markRunAttempt(int runIndex) {
    auto fields = m_fields.self();
    if (runIndex < 0 || runIndex >= static_cast<int>(fields->m_runStats.size())) {
        return;
    }

    if (fields->m_activeRunIdx == runIndex) {
        return;
    }

    fields->m_activeRunIdx = runIndex;
    auto& runStats = fields->m_runStats[runIndex];
    runStats.m_attempts += 1;
    runStats.m_bestReach = std::max(runStats.m_bestReach, runStats.m_startPercent);
}

void HookPlayLayer::recordBestReach(int runIndex, float percent) {
    auto fields = m_fields.self();
    if (runIndex < 0 || runIndex >= static_cast<int>(fields->m_runStats.size())) {
        return;
    }

    fields->m_runStats[runIndex].m_bestReach = std::max(
        fields->m_runStats[runIndex].m_bestReach,
        clampPercent(percent)
    );
}
