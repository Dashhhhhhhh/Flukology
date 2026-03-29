#pragma once

struct HookPlayLayer;

namespace flukology::learn_mode {
    void applyPendingStartPos(HookPlayLayer* playLayer);
    void onRunPassed(HookPlayLayer* playLayer, int runIndex);
    void onDeath(HookPlayLayer* playLayer, float deathPercent);
    void onLevelComplete(HookPlayLayer* playLayer);
}
