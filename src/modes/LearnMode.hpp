#pragma once

struct HookPlayLayer;

namespace dashcoach::learn_mode {
    void applyPendingStartPos(HookPlayLayer* playLayer);
    void onRunPassed(HookPlayLayer* playLayer);
    void onAttemptFinished(HookPlayLayer* playLayer);
}
