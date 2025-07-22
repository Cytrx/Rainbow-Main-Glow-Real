#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>
#include <chrono>
#include <functional>
#include <map>
#include <string>

using namespace geode::prelude;
using namespace std::chrono;

// Structure to hold mod settings
struct RainbowSettings {
    double speed;
    double saturation;
    double brightness;
    int64_t offset_color_p1;
    int64_t offset_color_p2;
    bool enable;
    bool glow;
    int64_t preset;
    int64_t playerPreset;
    bool sync;
    bool wave;
    bool superSpeed;
    bool pastel;
    bool editorEnable; // Added for editor specific enable
};

// Fetches all settings at once
RainbowSettings getModSettings() {
    auto mod = Mod::get();
    return {
        mod->getSettingValue<double>("speed"),
        mod->getSettingValue<double>("saturation"),
        mod->getSettingValue<double>("brightness"),
        mod->getSettingValue<int64_t>("offset_color_p1"),
        mod->getSettingValue<int64_t>("offset_color_p2"),
        mod->getSettingValue<bool>("enable"),
        mod->getSettingValue<bool>("glow"),
        mod->getSettingValue<int64_t>("preset"),
        mod->getSettingValue<int64_t>("playerPreset"),
        mod->getSettingValue<bool>("sync"),
        mod->getSettingValue<bool>("wave"),
        mod->getSettingValue<bool>("superSpeed"),
        mod->getSettingValue<bool>("pastel"),
        mod->getSettingValue<bool>("editorEnable") // Fetch editor setting
    };
}

void HSVtoRGB(float &r, float &g, float &b, float h, float s, float v)
{
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;

    if (h < 60)
    {
        r = c;
        g = x;
        b = 0;
    }
    else if (h < 120)
    {
        r = x;
        g = c;
        b = 0;
    }
    else if (h < 180)
    {
        r = 0;
        g = c;
        b = x;
    }
    else if (h < 240)
    {
        r = 0;
        g = x;
        b = c;
    }
    else if (h < 300)
    {
        r = x;
        g = 0;
        b = c;
    }
    else
    {
        r = c;
        g = 0;
        b = x;
    }

    r += m;
    g += m;
    b += m;
}

float g_hue = 0; // Renamed global variable for clarity

cocos2d::_ccColor3B getRainbow(float offset, float saturation, float value)
{
    float r, g, b;
    HSVtoRGB(r, g, b, fmod(g_hue + offset, 360), saturation / 100.0, value / 100.0);

    cocos2d::_ccColor3B out;
    out.r = static_cast<unsigned char>(r * 255);
    out.g = static_cast<unsigned char>(g * 255);
    out.b = static_cast<unsigned char>(b * 255);
    return out;
}

// Helper function to apply colors to a player
void applyRainbowColors(PlayerObject* player, bool isPlayer1, RainbowSettings& settings, const cocos2d::_ccColor3B& mainColor, const cocos2d::_ccColor3B& invertedColor) {
    if (!player) return;

    int64_t playerPreset = settings.playerPreset;
    bool applyToThisPlayer = (playerPreset == 1) || (isPlayer1 && playerPreset == 2) || (!isPlayer1 && playerPreset == 3);

    if (!applyToThisPlayer) return;

    // Apply Wave Trail Color
    if (settings.wave && player->m_waveTrail) {
        player->m_waveTrail->setColor(mainColor);
    }

    // Apply Main Color (rainbow)
    player->setColor(mainColor);

    // **Do NOT change secondary color** — keep it as original, so no call to setSecondColor

    // Apply Glow Color (rainbow)
    if (settings.glow) {
        player->m_glowColor = settings.sync ? mainColor : invertedColor;
        player->updateGlowColor();
    }
}

// Helper function to update hue
void updateHue(const RainbowSettings& settings) {
    if (g_hue >= 360)
    {
        g_hue = 0;
    }
    else
    {
        g_hue += settings.superSpeed ? 10.0f : static_cast<float>(settings.speed) / 10.0f;
    }
}

class $modify(PlayerObject)
{
    // Removed flashPlayer method (already done)
};

class $modify(MyPlayLayer, PlayLayer)
{
    void postUpdate(float p0)
    {
        PlayLayer::postUpdate(p0);

        auto settings = getModSettings();
        updateHue(settings);

        if (settings.enable)
        {
            if (settings.pastel)
            {
                settings.saturation = 50;
                settings.brightness = 90;
            }

            auto mainColorP1 = getRainbow(settings.offset_color_p1, settings.saturation, settings.brightness);
            auto invertedColorP1 = getRainbow(settings.offset_color_p1 + 180, settings.saturation, settings.brightness);
            auto mainColorP2 = getRainbow(settings.offset_color_p2, settings.saturation, settings.brightness);
            auto invertedColorP2 = getRainbow(settings.offset_color_p2 + 180, settings.saturation, settings.brightness);

            applyRainbowColors(m_player1, true, settings, mainColorP1, invertedColorP1);
            applyRainbowColors(m_player2, false, settings, mainColorP2, invertedColorP2);
        }
    }
};

class $modify(MyLevelEditorLayer, LevelEditorLayer)
{
    void postUpdate(float p0)
    {
        LevelEditorLayer::postUpdate(p0);

        auto settings = getModSettings();
        if (!settings.editorEnable) return;

        updateHue(settings);

        if (settings.pastel)
        {
            settings.saturation = 50;
            settings.brightness = 90;
        }

        auto mainColorP1 = getRainbow(settings.offset_color_p1, settings.saturation, settings.brightness);
        auto invertedColorP1 = getRainbow(settings.offset_color_p1 + 180, settings.saturation, settings.brightness);
        auto mainColorP2 = getRainbow(settings.offset_color_p2, settings.saturation, settings.brightness);
        auto invertedColorP2 = getRainbow(settings.offset_color_p2 + 180, settings.saturation, settings.brightness);

        applyRainbowColors(m_player1, true, settings, mainColorP1, invertedColorP1);
        if (m_player2) {
            applyRainbowColors(m_player2, false, settings, mainColorP2, invertedColorP2);
        }
    }
};

class $modify(SettingsBTN, EditorPauseLayer)
{
    void btnSettings(CCObject *)
    {
        geode::openSettingsPopup(Mod::get());
    };
    bool init(LevelEditorLayer *lel)
    {
        if (!EditorPauseLayer::init(lel))
            return false;

        bool shortcut = Mod::get()->getSettingValue<bool>("shortcut");

        if (shortcut)
        {
            auto btnSprite = CCSprite::create("btnSprite.png"_spr);
            auto menu = this->getChildByID("guidelines-menu");
            if (menu && btnSprite) {
                auto btn = CCMenuItemSpriteExtra::create(
                    btnSprite, this, menu_selector(SettingsBTN::btnSettings));
                btn->setID("settings-button"_spr);
                btn->setZOrder(10);
                menu->addChild(btn);
                menu->updateLayout();
            }
        }

        return true;
    }
};

class $modify(OpenSettings, PauseLayer)
{
    void btnSettings(CCObject *)
    {
        geode::openSettingsPopup(Mod::get());
    };

    void customSetup()
    {
        PauseLayer::customSetup();

        bool shortcut = Mod::get()->getSettingValue<bool>("shortcut");

        if (shortcut)
        {
            auto btnSprite = CCSprite::create("btnSprite.png"_spr);
            auto menu = this->getChildByID("right-button-menu");
            if (menu && btnSprite) {
                auto btn = CCMenuItemSpriteExtra::create(
                    btnSprite, this, menu_selector(OpenSettings::btnSettings));
                btn->setID("settings-button"_spr);
                btn->setZOrder(10);
                menu->addChild(btn);
                menu->updateLayout();
            }
        }
    };
};
