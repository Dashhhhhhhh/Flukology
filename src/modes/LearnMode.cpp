#include "LearnMode.hpp"

#include "hooks/PlayLayer.hpp"
#include "hooks/UILayer.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace dashcoach::learn_mode {
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
    }

    void onRunPassed(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        if (fields->m_learnModeEnabled) {
            fields->m_learnAdvancePending = true;
        }
    }

    void onAttemptFinished(HookPlayLayer* playLayer) {
        if (!playLayer) {
            return;
        }

        auto fields = playLayer->m_fields.self();
        if (!fields->m_learnModeEnabled || !fields->m_learnAdvancePending) {
            return;
        }

        fields->m_learnAdvancePending = false;
        fields->m_pendingStartPosIdx = std::max(fields->m_startPosIdx - 1, 0);
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
    fields->m_learnAdvancePending = false;
    fields->m_pendingStartPosIdx = -1;

    if (enabled && !fields->m_startPosObjects.empty()) {
        updateStartPos(static_cast<int>(fields->m_startPosObjects.size()));
    }

    if (auto* uiLayer = typeinfo_cast<UILayer*>(m_uiLayer)) {
        static_cast<HookUILayer*>(uiLayer)->updateUI();
    }
}

void HookPlayLayer::queuePreviousLearnStartPos() {
    dashcoach::learn_mode::onAttemptFinished(this);
}
