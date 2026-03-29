#include "PauseLayer.hpp"

#include "PlayLayer.hpp"

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <Geode/ui/TextArea.hpp>

#include <algorithm>
#include <cmath>
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

    class DashCoachStatsPopup final : public Popup {
    protected:
        std::vector<RunStats> m_runStats;

        bool init(std::vector<RunStats> runStats) {
            if (!Popup::init(320.f, 255.f)) {
                return false;
            }

            m_runStats = std::move(runStats);
            setTitle("DashCoach Runs");

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
        static DashCoachStatsPopup* create(std::vector<RunStats> runStats) {
            auto* popup = new DashCoachStatsPopup();
            if (popup->init(std::move(runStats))) {
                popup->autorelease();
                return popup;
            }

            delete popup;
            return nullptr;
        }
    };

    class DashCoachModesPopup final : public Popup {
    protected:
        bool init() {
            if (!Popup::init(220.f, 150.f)) {
                return false;
            }

            setTitle("Modes");

            auto subtitle = CCLabelBMFont::create("Choose a DashCoach mode", "chatFont.fnt");
            subtitle->setScale(0.7f);
            m_mainLayer->addChildAtPosition(subtitle, Anchor::Top, ccp(0.f, -40.f));

            auto buttonMenu = CCMenu::create();
            buttonMenu->setContentSize({ 180.f, 60.f });
            buttonMenu->setLayout(ColumnLayout::create()->setGap(10.f));
            m_mainLayer->addChildAtPosition(buttonMenu, Anchor::Center, ccp(0.f, -6.f));

            auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
            auto learnLabel = playLayer && playLayer->isLearnModeEnabled() ? "Learn: On" : "Learn";

            auto learnButton = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create(learnLabel),
                [this](CCObject*) {
                    if (auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get())) {
                        playLayer->toggleLearnMode();
                    }
                    onClose(nullptr);
                }
            );
            learnButton->setID("dashcoach-modes-learn-button"_spr);
            buttonMenu->addChild(learnButton);
            buttonMenu->updateLayout();

            return true;
        }

    public:
        static DashCoachModesPopup* create() {
            auto* popup = new DashCoachModesPopup();
            if (popup->init()) {
                popup->autorelease();
                return popup;
            }

            delete popup;
            return nullptr;
        }
    };

    class DashCoachMenuPopup final : public Popup {
    protected:
        std::vector<RunStats> m_runStats;

        bool init(std::vector<RunStats> runStats) {
            if (!Popup::init(220.f, 150.f)) {
                return false;
            }

            m_runStats = std::move(runStats);
            setTitle("DashCoach");

            auto subtitle = CCLabelBMFont::create("Choose a DashCoach tool", "chatFont.fnt");
            subtitle->setScale(0.7f);
            m_mainLayer->addChildAtPosition(subtitle, Anchor::Top, ccp(0.f, -40.f));

            auto buttonMenu = CCMenu::create();
            buttonMenu->setContentSize({ 180.f, 90.f });
            buttonMenu->setLayout(ColumnLayout::create()->setGap(10.f));
            m_mainLayer->addChildAtPosition(buttonMenu, Anchor::Center, ccp(0.f, -6.f));

            auto statsButton = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Stats"),
                [runStats = m_runStats](CCObject*) mutable {
                    DashCoachStatsPopup::create(std::move(runStats))->show();
                }
            );
            statsButton->setID("dashcoach-stats-button"_spr);
            buttonMenu->addChild(statsButton);

            auto modesButton = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Modes"),
                [](CCObject*) {
                    DashCoachModesPopup::create()->show();
                }
            );
            modesButton->setID("dashcoach-modes-button"_spr);
            buttonMenu->addChild(modesButton);

            buttonMenu->updateLayout();

            return true;
        }

    public:
        static DashCoachMenuPopup* create(std::vector<RunStats> runStats) {
            auto* popup = new DashCoachMenuPopup();
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
    menu->setID("dashcoach-pause-menu"_spr);
    menu->setPosition(winSize.width / 2.f, 72.f);

    auto buttonSprite = ButtonSprite::create("DashCoach");
    auto button = CCMenuItemExt::createSpriteExtra(buttonSprite, [](CCObject*) {
        if (auto* playLayer = static_cast<HookPlayLayer*>(PlayLayer::get())) {
            DashCoachMenuPopup::create(playLayer->getRunStats())->show();
        }
    });

    button->setID("dashcoach-button"_spr);
    menu->addChild(button);
    this->addChild(menu);
}
