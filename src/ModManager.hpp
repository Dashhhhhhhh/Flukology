#pragma once

class ModManager {
public:
    static ModManager* sharedState();

    ModManager();

    bool m_dontFadeOnStart = false;
    bool m_hideBtns = false;
    bool m_ignoreDisabled = false;
    double m_opacity = 0;
    bool m_startposPracticeEnabled = false;
    float m_startposPracticeThreshold = 30.f;

    bool isStartposPracticeEnabled() const;
    void setStartposPracticeEnabled(bool enabled);
    void toggleStartposPractice();
    float getStartposPracticeThreshold() const;
    void setStartposPracticeThreshold(float threshold);
};
