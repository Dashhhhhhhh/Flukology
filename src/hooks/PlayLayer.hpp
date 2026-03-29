#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

struct RunStats {
    float m_startPercent = 0.f;
    float m_endPercent = 100.f;
    int m_attempts = 0;
    int m_clears = 0;
    float m_bestReach = 0.f;
    std::vector<int> m_deathPercents;
};

struct HookPlayLayer : geode::Modify<HookPlayLayer, PlayLayer> {
    struct Fields {
        std::vector<geode::Ref<GameObject>> m_startPosObjects;
        std::vector<RunStats> m_runStats;
        int m_startPosIdx = 0;
        int m_activeRunIdx = -1;
        bool m_activeRunPassed = false;
    };

    void addObject(GameObject* obj);
    void createObjectsFromSetupFinished();
    void updateStartPos(int index);
    void resetLevel();
    void postUpdate(float dt);
    void destroyPlayer(PlayerObject* player, GameObject* object);
    void levelComplete();

    std::vector<RunStats> const& getRunStats();
    int findRunIndexForPercent(float percent);
    int getSelectedRunIndex();
    void rebuildRunStats();
    void resetActiveRunTracking();
    void markRunAttempt(int runIndex);
    void markRunPass(int runIndex);
    void recordBestReach(int runIndex, float percent);
};
