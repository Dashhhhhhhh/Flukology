#include "PauseLayer.hpp"

#include "PlayLayer.hpp"

#include <Geode/binding/CCMenuItemToggler.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <Geode/ui/TextArea.hpp>
#include <Geode/ui/TextInput.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>

using namespace geode::prelude;

namespace {
    struct ChainSegment {
        float m_startPercent = 0.f;
        float m_endPercent = 0.f;
    };

    int displayPercent(float percent) {
        return static_cast<int>(std::lround(std::clamp(percent, 0.f, 100.f)));
    }

    std::string formatThresholdValue(float threshold) {
        auto const clampedThreshold = std::clamp(threshold, 0.f, 100.f);
        if (std::abs(clampedThreshold - std::round(clampedThreshold)) < 0.001f) {
            return fmt::format("{}", static_cast<int>(std::lround(clampedThreshold)));
        }

        return fmt::format("{:.1f}", clampedThreshold);
    }

    std::optional<float> parseThresholdValue(std::string const& value) {
        if (value.empty()) {
            return std::nullopt;
        }

        char* parseEnd = nullptr;
        auto const parsedValue = std::strtof(value.c_str(), &parseEnd);
        if (parseEnd == value.c_str() || (parseEnd && *parseEnd != '\0') || !std::isfinite(parsedValue)) {
            return std::nullopt;
        }

        return std::clamp(parsedValue, 0.f, 100.f);
    }

    std::string formatRangeLabel(RunStats const& runStats) {
        return fmt::format("{}-{}%", displayPercent(runStats.m_startPercent), displayPercent(runStats.m_endPercent));
    }

    std::string formatCompactRangeLabel(RunStats const& runStats) {
        return fmt::format("{}-{}", displayPercent(runStats.m_startPercent), displayPercent(runStats.m_endPercent));
    }

    std::string formatCompactRangeLabel(ChainSegment const& segment) {
        return fmt::format("{}-{}", displayPercent(segment.m_startPercent), displayPercent(segment.m_endPercent));
    }

    std::string formatBestRun(RunStats const& runStats) {
        if (runStats.m_attempts == 0 && runStats.m_clears == 0) {
            return "-";
        }

        return fmt::format("{}%", displayPercent(runStats.m_bestReach));
    }

    std::optional<std::vector<ChainSegment>> buildCompletedRunChain(std::vector<RunStats> const& runStats) {
        std::vector<ChainSegment> achievedRuns;
        for (auto const& runStat : runStats) {
            if (runStat.m_bestReach > runStat.m_startPercent + 0.001f) {
                achievedRuns.push_back({
                    runStat.m_startPercent,
                    std::clamp(runStat.m_bestReach, runStat.m_startPercent, 100.f),
                });
            }
        }

        if (achievedRuns.empty()) {
            return std::nullopt;
        }

        std::sort(achievedRuns.begin(), achievedRuns.end(), [](ChainSegment const& left, ChainSegment const& right) {
            if (left.m_startPercent == right.m_startPercent) {
                return left.m_endPercent > right.m_endPercent;
            }
            return left.m_startPercent < right.m_startPercent;
        });

        std::vector<ChainSegment> chain;
        float coveredEnd = 0.f;
        size_t index = 0;

        while (index < achievedRuns.size()) {
            if (achievedRuns[index].m_startPercent > coveredEnd + 0.001f) {
                break;
            }

            size_t bestIndex = achievedRuns.size();
            float bestEnd = coveredEnd;

            while (index < achievedRuns.size() && achievedRuns[index].m_startPercent <= coveredEnd + 0.001f) {
                if (achievedRuns[index].m_endPercent > bestEnd + 0.001f) {
                    bestEnd = achievedRuns[index].m_endPercent;
                    bestIndex = index;
                }
                ++index;
            }

            if (bestIndex == achievedRuns.size()) {
                return std::nullopt;
            }

            chain.push_back(achievedRuns[bestIndex]);
            coveredEnd = bestEnd;

            if (coveredEnd >= 100.f - 0.001f) {
                return chain;
            }
        }

        return std::nullopt;
    }

    std::string formatRunsSummary(std::vector<RunStats> const& runStats) {
        auto chain = buildCompletedRunChain(runStats);
        if (!chain) {
            return "";
        }

        std::string result = fmt::format("In {} {} ", chain->size(), chain->size() == 1 ? "run" : "runs");
        for (size_t index = 0; index < chain->size(); ++index) {
            if (index != 0) {
                result += ", ";
            }
            result += formatCompactRangeLabel((*chain)[index]);
        }
        return result;
    }

    CCLayerColor* createRunRow(RunStats const& runStats) {
        constexpr float rowWidth = 250.f;

        auto details = SimpleTextArea::create(
            fmt::format(
                "Attempts: {} | Passes: {} | Best run: {}",
                runStats.m_attempts,
                runStats.m_clears,
                formatBestRun(runStats)
            ),
            "chatFont.fnt",
            0.6f,
            rowWidth - 20.f
        );
        details->setAlignment(kCCTextAlignmentLeft);

        auto rowHeight = std::max(44.f, details->getHeight() + 34.f);
        auto row = CCLayerColor::create(ccc4(0, 0, 0, 90), rowWidth, rowHeight);
        row->ignoreAnchorPointForPosition(false);

        auto title = CCLabelBMFont::create(formatRangeLabel(runStats).c_str(), "goldFont.fnt");
        title->setAnchorPoint({ 0.f, 1.f });
        title->setScale(0.55f);
        title->setPosition({ 10.f, rowHeight - 6.f });
        row->addChild(title);

        details->setAnchorPoint({ 0.f, 1.f });
        details->setPosition({ 10.f, rowHeight - 24.f });
        row->addChild(details);

        return row;
    }

    class FlukologyStatsPopup final : public Popup {
    protected:
        std::vector<RunStats> m_runStats;

        bool init(std::vector<RunStats> runStats) {
            if (!Popup::init(320.f, 255.f)) {
                return false;
            }

            m_runStats = std::move(runStats);
            setTitle("Flukology Runs");

            auto subtitle = CCLabelBMFont::create("Saved section stats for this level", "chatFont.fnt");
            subtitle->setScale(0.7f);
            m_mainLayer->addChildAtPosition(subtitle, Anchor::Top, ccp(0.f, -38.f));

            auto background = CCLayerColor::create(ccc4(0, 0, 0, 75), 270.f, 132.f);
            background->ignoreAnchorPointForPosition(false);
            m_mainLayer->addChildAtPosition(background, Anchor::Center, ccp(-6.f, 8.f));

            auto scrollLayer = ScrollLayer::create({ 270.f, 132.f });
            scrollLayer->ignoreAnchorPointForPosition(false);
            scrollLayer->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(6.f));
            m_mainLayer->addChildAtPosition(scrollLayer, Anchor::Center, ccp(-6.f, 8.f));
            m_mainLayer->addChildAtPosition(Scrollbar::create(scrollLayer), Anchor::Right, ccp(-12.f, 8.f));

            for (auto const& runStatsEntry : m_runStats) {
                scrollLayer->m_contentLayer->addChild(createRunRow(runStatsEntry));
            }

            scrollLayer->m_contentLayer->updateLayout();
            scrollLayer->scrollToTop();

            auto runsSummaryText = formatRunsSummary(m_runStats);
            if (!runsSummaryText.empty()) {
                auto runsSummary = SimpleTextArea::create(
                    runsSummaryText,
                    "chatFont.fnt",
                    0.6f,
                    270.f
                );
                runsSummary->setAlignment(kCCTextAlignmentCenter);
                m_mainLayer->addChildAtPosition(runsSummary, Anchor::Bottom, ccp(0.f, 24.f));
            }

            return true;
        }

    public:
        static FlukologyStatsPopup* create(std::vector<RunStats> runStats) {
            auto* popup = new FlukologyStatsPopup();
            if (popup->init(std::move(runStats))) {
                popup->autorelease();
                return popup;
            }

            delete popup;
            return nullptr;
        }
    };

    class FlukologyModesPopup final : public Popup {
    protected:
        TextInput* m_thresholdInput = nullptr;
        CCMenuItemToggler* m_learnToggle = nullptr;
        CCMenuItemToggler* m_useLastStartposToggle = nullptr;

        void refreshModeToggles() {
            auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
            if (!playLayer) {
                return;
            }

            if (m_learnToggle) {
                m_learnToggle->toggle(playLayer->isLearnModeEnabled());
            }

            if (m_useLastStartposToggle) {
                m_useLastStartposToggle->toggle(playLayer->isStartposPracticeModeEnabled());
            }
        }

        bool init() {
            if (!Popup::init(330.f, 250.f)) {
                return false;
            }

            setTitle("Modes");

            auto subtitle = CCLabelBMFont::create("Practice helpers for this level", "chatFont.fnt");
            subtitle->setScale(0.7f);
            m_mainLayer->addChildAtPosition(subtitle, Anchor::Top, ccp(0.f, -40.f));

            auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
            auto modesPanel = CCLayerColor::create(ccc4(0, 0, 0, 75), 276.f, 112.f);
            modesPanel->ignoreAnchorPointForPosition(false);
            m_mainLayer->addChildAtPosition(modesPanel, Anchor::Top, ccp(0.f, -70.f));

            auto modesTitle = CCLabelBMFont::create("Modes", "goldFont.fnt");
            modesTitle->setScale(0.45f);
            modesTitle->setPosition({ 138.f, 97.f });
            modesPanel->addChild(modesTitle);

            auto learnRow = CCLayerColor::create(ccc4(255, 255, 255, 18), 248.f, 32.f);
            learnRow->ignoreAnchorPointForPosition(false);
            learnRow->setPosition({ 14.f, 55.f });
            modesPanel->addChild(learnRow);

            auto learnLabel = CCLabelBMFont::create("Learn", "bigFont.fnt");
            learnLabel->setAnchorPoint({ 0.f, 0.5f });
            learnLabel->setScale(0.5f);
            learnLabel->setPosition({ 12.f, 16.f });
            learnRow->addChild(learnLabel);

            auto learnMenu = CCMenu::create();
            learnMenu->setPosition({ 223.f, 16.f });
            learnRow->addChild(learnMenu);

            m_learnToggle = CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [this](auto) {
                if (auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get())) {
                    playLayer->toggleLearnMode();
                }
                refreshModeToggles();
            });
            m_learnToggle->setID("flukology-modes-learn-toggle"_spr);
            learnMenu->addChild(m_learnToggle);

            auto useLastStartposRow = CCLayerColor::create(ccc4(255, 255, 255, 18), 248.f, 32.f);
            useLastStartposRow->ignoreAnchorPointForPosition(false);
            useLastStartposRow->setPosition({ 14.f, 18.f });
            modesPanel->addChild(useLastStartposRow);

            auto useLastStartposLabel = CCLabelBMFont::create("Use Last Startpos", "bigFont.fnt");
            useLastStartposLabel->setAnchorPoint({ 0.f, 0.5f });
            useLastStartposLabel->setScale(0.42f);
            useLastStartposLabel->setPosition({ 12.f, 16.f });
            useLastStartposRow->addChild(useLastStartposLabel);

            auto useLastStartposMenu = CCMenu::create();
            useLastStartposMenu->setPosition({ 223.f, 16.f });
            useLastStartposRow->addChild(useLastStartposMenu);

            m_useLastStartposToggle = CCMenuItemExt::createTogglerWithStandardSprites(0.6f, [this](auto) {
                if (auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get())) {
                    playLayer->toggleStartposPracticeMode();
                }
                refreshModeToggles();
            });
            m_useLastStartposToggle->setID("flukology-modes-use-last-startpos-toggle"_spr);
            useLastStartposMenu->addChild(m_useLastStartposToggle);

            auto thresholdPanel = CCLayerColor::create(ccc4(0, 0, 0, 75), 276.f, 90.f);
            thresholdPanel->ignoreAnchorPointForPosition(false);
            m_mainLayer->addChildAtPosition(thresholdPanel, Anchor::Bottom, ccp(0.f, 42.f));

            auto thresholdTitle = CCLabelBMFont::create("Use Last Startpos Threshold (%)", "goldFont.fnt");
            thresholdTitle->setScale(0.34f);
            thresholdTitle->setPosition({ 138.f, 72.f });
            thresholdPanel->addChild(thresholdTitle);

            m_thresholdInput = TextInput::create(96.f, "30", "bigFont.fnt");
            m_thresholdInput->setFilter("0123456789.");
            m_thresholdInput->setMaxCharCount(5);
            m_thresholdInput->setPosition({ 92.f, 41.f });
            if (playLayer) {
                m_thresholdInput->setString(formatThresholdValue(playLayer->getStartposPracticeThreshold()), false);
            }
            thresholdPanel->addChild(m_thresholdInput);

            auto applyMenu = CCMenu::create();
            applyMenu->setPosition({ 208.f, 41.f });
            thresholdPanel->addChild(applyMenu);

            auto applyThresholdButton = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Apply"),
                [this](CCObject*) {
                    auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
                    if (!playLayer || !m_thresholdInput) {
                        return;
                    }

                    auto thresholdValue = parseThresholdValue(m_thresholdInput->getString());
                    if (!thresholdValue) {
                        FLAlertLayer::create("Flukology", "Enter a valid threshold from 0 to 100.", "OK")->show();
                        return;
                    }

                    playLayer->setStartposPracticeThreshold(*thresholdValue);
                    m_thresholdInput->setString(formatThresholdValue(*thresholdValue), false);
                }
            );
            applyThresholdButton->setID("flukology-modes-threshold-apply-button"_spr);
            applyMenu->addChild(applyThresholdButton);

            auto thresholdNote = SimpleTextArea::create(
                "Learn mode phase 3 uses this too.",
                "chatFont.fnt",
                0.55f,
                236.f
            );
            thresholdNote->setAlignment(kCCTextAlignmentCenter);
            thresholdNote->setPosition({ 138.f, 14.f });
            thresholdPanel->addChild(thresholdNote);

            refreshModeToggles();

            return true;
        }

    public:
        static FlukologyModesPopup* create() {
            auto* popup = new FlukologyModesPopup();
            if (popup->init()) {
                popup->autorelease();
                return popup;
            }

            delete popup;
            return nullptr;
        }
    };

    class FlukologyMenuPopup final : public Popup {
    protected:
        std::vector<RunStats> m_runStats;

        bool init(std::vector<RunStats> runStats) {
            if (!Popup::init(220.f, 150.f)) {
                return false;
            }

            m_runStats = std::move(runStats);
            setTitle("Flukology");

            auto subtitle = CCLabelBMFont::create("Choose a Flukology tool", "chatFont.fnt");
            subtitle->setScale(0.7f);
            m_mainLayer->addChildAtPosition(subtitle, Anchor::Top, ccp(0.f, -40.f));

            auto buttonMenu = CCMenu::create();
            buttonMenu->setContentSize({ 180.f, 90.f });
            buttonMenu->setLayout(ColumnLayout::create()->setGap(10.f));
            m_mainLayer->addChildAtPosition(buttonMenu, Anchor::Center, ccp(0.f, -6.f));

            auto statsButton = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Stats"),
                [runStats = m_runStats](CCObject*) mutable {
                    FlukologyStatsPopup::create(std::move(runStats))->show();
                }
            );
            statsButton->setID("flukology-stats-button"_spr);
            buttonMenu->addChild(statsButton);

            auto modesButton = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Modes"),
                [](CCObject*) {
                    FlukologyModesPopup::create()->show();
                }
            );
            modesButton->setID("flukology-modes-button"_spr);
            buttonMenu->addChild(modesButton);

            buttonMenu->updateLayout();

            return true;
        }

    public:
        static FlukologyMenuPopup* create(std::vector<RunStats> runStats) {
            auto* popup = new FlukologyMenuPopup();
            if (popup->init(std::move(runStats))) {
                popup->autorelease();
                return popup;
            }

            delete popup;
            return nullptr;
        }
    };
}

void HookPauseLayer::customSetup() {
    PauseLayer::customSetup();

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto menu = CCMenu::create();
    menu->setID("flukology-pause-menu"_spr);
    menu->setPosition(winSize.width / 2.f, 72.f);

    auto buttonSprite = ButtonSprite::create("Flukology");
    auto button = CCMenuItemExt::createSpriteExtra(buttonSprite, [](CCObject*) {
        if (auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get())) {
            FlukologyMenuPopup::create(playLayer->getRunStats())->show();
        }
    });

    button->setID("flukology-button"_spr);
    menu->addChild(button);
    this->addChild(menu);
}
