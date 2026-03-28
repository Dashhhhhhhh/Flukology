#include "ModManager.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

ModManager* ModManager::sharedState() {
    static ModManager instance;
    return &instance;
}

ModManager::ModManager() {
    m_dontFadeOnStart = Mod::get()->getSettingValue<bool>("hide");
    m_hideBtns = Mod::get()->getSettingValue<bool>("hideBtns");
    m_ignoreDisabled = Mod::get()->getSettingValue<bool>("ignoreDisabled");
    m_opacity = Mod::get()->getSettingValue<double>("opacity") / 100 * 255;
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
