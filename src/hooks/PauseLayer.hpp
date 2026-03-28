#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

struct HookPauseLayer : geode::Modify<HookPauseLayer, PauseLayer> {
    void customSetup();
};
