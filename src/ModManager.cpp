#include "ModManager.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace {
    constexpr auto kStartposPracticeEnabledKey = "startpos-practice-enabled";
    constexpr auto kStartposPracticeThresholdKey = "startpos-practice-threshold";

    float clampStartposPracticeThreshold(float threshold) {
        return std::clamp(threshold, 0.f, 100.f);
    }
}

ModManager* ModManager::sharedState() {
    static ModManager instance;
    return &instance;
}

ModManager::ModManager() {
    m_dontFadeOnStart = Mod::get()->getSettingValue<bool>("hide");
    m_hideBtns = Mod::get()->getSettingValue<bool>("hideBtns");
    m_ignoreDisabled = Mod::get()->getSettingValue<bool>("ignoreDisabled");
    m_opacity = Mod::get()->getSettingValue<double>("opacity") / 100 * 255;
    m_startposPracticeEnabled = Mod::get()->getSavedValue<bool>(kStartposPracticeEnabledKey, false);
    m_startposPracticeThreshold = clampStartposPracticeThreshold(
        static_cast<float>(Mod::get()->getSavedValue<double>(kStartposPracticeThresholdKey, 30.0))
    );
}

bool ModManager::isStartposPracticeEnabled() const {
    return m_startposPracticeEnabled;
}

void ModManager::setStartposPracticeEnabled(bool enabled) {
    m_startposPracticeEnabled = enabled;
    Mod::get()->setSavedValue(kStartposPracticeEnabledKey, enabled);
}

void ModManager::toggleStartposPractice() {
    setStartposPracticeEnabled(!m_startposPracticeEnabled);
}

float ModManager::getStartposPracticeThreshold() const {
    return m_startposPracticeThreshold;
}

void ModManager::setStartposPracticeThreshold(float threshold) {
    m_startposPracticeThreshold = clampStartposPracticeThreshold(threshold);
    Mod::get()->setSavedValue(kStartposPracticeThresholdKey, static_cast<double>(m_startposPracticeThreshold));
}

$on_mod(Loaded) {
    auto manager = ModManager::sharedState();

    listenForSettingChanges<bool>("hide", [manager](bool value) {
        manager->m_dontFadeOnStart = value;
    });

    listenForSettingChanges<bool>("hideBtns", [manager](bool value) {
        manager->m_hideBtns = value;
    });

    listenForSettingChanges<bool>("ignoreDisabled", [manager](bool value) {
        manager->m_ignoreDisabled = value;
    });

    listenForSettingChanges<double>("opacity", [manager](double value) {
        manager->m_opacity = value / 100 * 255;
    });
}
