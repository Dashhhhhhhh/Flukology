#include "PauseLayer.hpp"

#include "PlayLayer.hpp"

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <Geode/ui/TextArea.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    int displayPercent(float percent) {
        return static_cast<int>(std::lround(std::clamp(percent, 0.f, 100.f)));
    }

    std::string formatRangeLabel(RunStats const& runStats) {
        return fmt::format("{}-{}%", displayPercent(runStats.m_startPercent), displayPercent(runStats.m_endPercent));
    }

    std::string formatBestRun(RunStats const& runStats) {
        if (runStats.m_attempts == 0 && runStats.m_clears == 0) {
            return "-";
        }

        return fmt::format("{}%", displayPercent(runStats.m_bestReach));
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
            if (!Popup::init(320.f, 240.f)) {
                return false;
            }

            m_runStats = std::move(runStats);
            setTitle("DashCoach Runs");

            auto subtitle = CCLabelBMFont::create("Current session section stats", "chatFont.fnt");
            subtitle->setScale(0.7f);
            m_mainLayer->addChildAtPosition(subtitle, Anchor::Top, ccp(0.f, -38.f));

            auto background = CCLayerColor::create(ccc4(0, 0, 0, 75), 270.f, 150.f);
            background->ignoreAnchorPointForPosition(false);
            m_mainLayer->addChildAtPosition(background, Anchor::Center, ccp(-6.f, -2.f));

            auto scrollLayer = ScrollLayer::create({ 270.f, 150.f });
            scrollLayer->ignoreAnchorPointForPosition(false);
            scrollLayer->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(6.f));
            m_mainLayer->addChildAtPosition(scrollLayer, Anchor::Center, ccp(-6.f, -2.f));
            m_mainLayer->addChildAtPosition(Scrollbar::create(scrollLayer), Anchor::Right, ccp(-12.f, -2.f));

            for (auto const& runStatsEntry : m_runStats) {
                scrollLayer->m_contentLayer->addChild(createRunRow(runStatsEntry));
            }

            scrollLayer->m_contentLayer->updateLayout();
            scrollLayer->scrollToTop();

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
            DashCoachStatsPopup::create(playLayer->getRunStats())->show();
        }
    });

    button->setID("dashcoach-button"_spr);
    menu->addChild(button);
    this->addChild(menu);
}
