#include "LearnMode.hpp"

#include "../ModManager.hpp"
#include "hooks/PlayLayer.hpp"
#include "hooks/UILayer.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    constexpr float kLearnPercentEpsilon = 0.001f;

    float getStartPosPercent(HookPlayLayer* playLayer, int startPosIdx) {
        if (!playLayer || startPosIdx <= 0) {
            return 0.f;
        }

        auto fields = playLayer->m_fields.self();
        if (startPosIdx > static_cast<int>(fields->m_startPosObjects.size())) {
            return 100.f;
        }

        auto const levelLength = std::max(playLayer->m_levelLength, 1.f);
        return std::clamp(
            fields->m_startPosObjects[startPosIdx - 1]->getPositionX() / levelLength * 100.f,
            0.f,
            100.f
        );
    }

    int getSecondToLastStartPosIdx(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return 0;
        }

        auto const count = static_cast<int>(playLayer->m_fields->m_startPosObjects.size());
        return count >= 2 ? count - 1 : 0;
    }

    bool isStartPosBefore50(HookPlayLayer* playLayer, int startPosIdx) {
        return getStartPosPercent(playLayer, startPosIdx) < 50.f - kLearnPercentEpsilon;
    }

    bool haveAllIntroRunsBeenPassed(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return false;
        }

        auto fields = playLayer->m_fields.self();
        if (fields->m_learnPassedRuns.size() != fields->m_runStats.size()) {
            return false;
        }

        return std::ranges::all_of(fields->m_learnPassedRuns, [](bool value) {
            return value;
        });
    }

    void startStartposPracticePhase(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        fields->m_learnStage = LearnModeStage::StartposPractice;
        fields->m_pendingStartPosIdx = 0;
        playLayer->savePersistentLearnMode();
    }

    void enterPostIntroStage(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        auto const secondToLastStartPosIdx = getSecondToLastStartPosIdx(playLayer);
        if (secondToLastStartPosIdx <= 0 || isStartPosBefore50(playLayer, secondToLastStartPosIdx)) {
            startStartposPracticePhase(playLayer);
            return;
        }

        fields->m_learnStage = LearnModeStage::CompletionBacktrack;
        fields->m_pendingStartPosIdx = secondToLastStartPosIdx;
        playLayer->savePersistentLearnMode();
    }

    int getStartPosBeforePercent(HookPlayLayer* playLayer, float percent) {
        if (!playLayer) {
            return 0;
        }

        auto fields = playLayer->m_fields.self();
        auto const clampedPercent = std::clamp(percent, 0.f, 100.f);
        auto selectedIdx = 0;

        for (auto index = 0; index < static_cast<int>(fields->m_startPosObjects.size()); ++index) {
            auto const objectPercent = getStartPosPercent(playLayer, index + 1);
            if (objectPercent < clampedPercent - kLearnPercentEpsilon) {
                selectedIdx = index + 1;
                continue;
            }
            break;
        }

        return selectedIdx;
    }

    bool shouldApplyStartposPractice(HookPlayLayer* playLayer, float deathPercent) {
        if (!playLayer) {
            return false;
        }

        auto const learnPhaseActive = playLayer->isLearnModeInStartposPracticeStage();
        auto const standaloneModeActive = !playLayer->isLearnModeEnabled() && playLayer->isStartposPracticeModeEnabled();
        if (!learnPhaseActive && !standaloneModeActive) {
            return false;
        }

        auto const currentStartPosIdx = playLayer->m_fields->m_startPosIdx;
        if (currentStartPosIdx > 0) {
            return true;
        }

        return deathPercent + kLearnPercentEpsilon >= playLayer->getStartposPracticeThreshold();
    }
}

namespace flukology::learn_mode {
    void applyPendingStartPos(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        if (fields->m_pendingStartPosIdx < 0) {
            return;
        }

        playLayer->setSelectedStartPos(fields->m_pendingStartPosIdx);
        fields->m_pendingStartPosIdx = -1;
        if (fields->m_learnModeEnabled) {
            fields->m_learnResumeStartPosIdx = playLayer->m_fields->m_startPosIdx;
            playLayer->savePersistentLearnMode();
        }
    }

    void onRunPassed(HookPlayLayer* playLayer, int runIndex) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        if (!fields->m_learnModeEnabled) {
            return;
        }

        if (fields->m_learnStage != LearnModeStage::SectionRuns) {
            return;
        }

        if (runIndex >= 0 && runIndex < static_cast<int>(fields->m_learnPassedRuns.size())) {
            fields->m_learnPassedRuns[runIndex] = true;
        }

        if (haveAllIntroRunsBeenPassed(playLayer)) {
            enterPostIntroStage(playLayer);
            return;
        }

        fields->m_pendingStartPosIdx = std::max(fields->m_startPosIdx - 1, 0);
        playLayer->savePersistentLearnMode();
    }

    void onDeath(HookPlayLayer* playLayer, float deathPercent) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        if (fields->m_learnModeEnabled && fields->m_learnStage == LearnModeStage::CompletionBacktrack) {
            if (fields->m_pendingStartPosIdx >= 0) {
                return;
            }
            fields->m_pendingStartPosIdx = fields->m_startPosIdx;
            playLayer->savePersistentLearnMode();
            return;
        }

        if (!shouldApplyStartposPractice(playLayer, deathPercent)) {
            return;
        }

        fields->m_pendingStartPosIdx = getStartPosBeforePercent(playLayer, deathPercent);
        if (fields->m_learnModeEnabled) {
            playLayer->savePersistentLearnMode();
        }
    }

    void onLevelComplete(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        if (fields->m_learnModeEnabled && fields->m_learnStage == LearnModeStage::CompletionBacktrack) {
            auto const previousStartPosIdx = std::max(fields->m_startPosIdx - 1, 0);
            if (previousStartPosIdx <= 0 || isStartPosBefore50(playLayer, previousStartPosIdx)) {
                startStartposPracticePhase(playLayer);
                return;
            }

            fields->m_pendingStartPosIdx = previousStartPosIdx;
            playLayer->savePersistentLearnMode();
            return;
        }

        if (fields->m_learnModeEnabled && fields->m_learnStage == LearnModeStage::StartposPractice) {
            fields->m_pendingStartPosIdx = 0;
            playLayer->savePersistentLearnMode();
            return;
        }

        if (!fields->m_learnModeEnabled && playLayer->isStartposPracticeModeEnabled() && fields->m_startPosIdx > 0) {
            fields->m_pendingStartPosIdx = 0;
        }
    }
}

bool HookPlayLayer::isLearnModeEnabled() {
    return m_fields->m_learnModeEnabled;
}

void HookPlayLayer::toggleLearnMode() {
    setLearnModeEnabled(!m_fields->m_learnModeEnabled);
}

void HookPlayLayer::setLearnModeEnabled(bool enabled) {
    auto fields = m_fields.self();
    fields->m_learnModeEnabled = enabled;
    fields->m_pendingStartPosIdx = -1;

    if (enabled) {
        if (!fields->m_hasPersistentLearnModeState) {
            fields->m_learnStage = LearnModeStage::SectionRuns;
            fields->m_learnPassedRuns.assign(fields->m_runStats.size(), false);
            fields->m_learnResumeStartPosIdx = static_cast<int>(fields->m_startPosObjects.size());
        }
        else if (fields->m_learnPassedRuns.size() != fields->m_runStats.size()) {
            fields->m_learnStage = LearnModeStage::SectionRuns;
            fields->m_learnPassedRuns.assign(fields->m_runStats.size(), false);
            fields->m_learnResumeStartPosIdx = static_cast<int>(fields->m_startPosObjects.size());
        }

        savePersistentLearnMode();
        if (!fields->m_startPosObjects.empty()) {
            updateStartPos(fields->m_learnResumeStartPosIdx);
            return;
        }
    }

    savePersistentLearnMode();

    if (auto* uiLayer = typeinfo_cast<UILayer*>(m_uiLayer)) {
        static_cast<HookUILayer*>(uiLayer)->updateUI();
    }
}

bool HookPlayLayer::isStartposPracticeModeEnabled() {
    return ModManager::sharedState()->isStartposPracticeEnabled();
}

void HookPlayLayer::toggleStartposPracticeMode() {
    setStartposPracticeModeEnabled(!isStartposPracticeModeEnabled());
}

void HookPlayLayer::setStartposPracticeModeEnabled(bool enabled) {
    ModManager::sharedState()->setStartposPracticeEnabled(enabled);
}

float HookPlayLayer::getStartposPracticeThreshold() {
    return ModManager::sharedState()->getStartposPracticeThreshold();
}

void HookPlayLayer::setStartposPracticeThreshold(float threshold) {
    ModManager::sharedState()->setStartposPracticeThreshold(threshold);
}

bool HookPlayLayer::isLearnModeInStartposPracticeStage() {
    return m_fields->m_learnModeEnabled && m_fields->m_learnStage == LearnModeStage::StartposPractice;
}
