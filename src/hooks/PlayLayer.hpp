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

enum class LearnModeStage {
    SectionRuns,
    CompletionBacktrack,
    StartposPractice,
};

struct LearnModeProgress {
    bool m_enabled = false;
    LearnModeStage m_stage = LearnModeStage::SectionRuns;
    std::vector<int> m_passedRuns;
    int m_resumeStartPosIdx = 0;
};

struct HookPlayLayer : geode::Modify<HookPlayLayer, PlayLayer> {
    struct Fields {
        std::vector<geode::Ref<GameObject>> m_startPosObjects;
        std::vector<RunStats> m_runStats;
        std::string m_runStatsSaveKey;
        std::string m_learnModeSaveKey;
        int m_startPosIdx = 0;
        int m_activeRunIdx = -1;
        bool m_activeRunPassed = false;
        bool m_learnModeEnabled = false;
        LearnModeStage m_learnStage = LearnModeStage::SectionRuns;
        std::vector<bool> m_learnPassedRuns;
        int m_learnResumeStartPosIdx = 0;
        bool m_hasPersistentLearnModeState = false;
        int m_pendingStartPosIdx = -1;
    };

    void addObject(GameObject* obj);
    void createObjectsFromSetupFinished();
    void updateStartPos(int index);
    void setSelectedStartPos(int index);
    void resetLevel();
    void postUpdate(float dt);
    void destroyPlayer(PlayerObject* player, GameObject* object);
    void levelComplete();

    std::vector<RunStats> const& getRunStats();
    int findRunIndexForPercent(float percent);
    int getSelectedRunIndex();
    bool isLearnModeEnabled();
    void toggleLearnMode();
    void setLearnModeEnabled(bool enabled);
    bool isStartposPracticeModeEnabled();
    void toggleStartposPracticeMode();
    void setStartposPracticeModeEnabled(bool enabled);
    float getStartposPracticeThreshold();
    void setStartposPracticeThreshold(float threshold);
    bool isLearnModeInStartposPracticeStage();
    std::string getRunStatsSaveKey();
    void loadPersistentRunStats();
    void savePersistentRunStats();
    std::string getLearnModeSaveKey();
    void loadPersistentLearnMode();
    void savePersistentLearnMode();
    void rebuildRunStats();
    void resetActiveRunTracking();
    void markRunAttempt(int runIndex);
    void markRunPass(int runIndex);
    void recordBestReach(int runIndex, float percent);
};
