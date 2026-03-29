#pragma once

struct HookPlayLayer;

namespace dashcoach::learn_mode {
    void applyPendingStartPos(HookPlayLayer* playLayer);
    void onRunPassed(HookPlayLayer* playLayer, int runIndex);
    void onDeath(HookPlayLayer* playLayer, float deathPercent);
    void onLevelComplete(HookPlayLayer* playLayer);
}
