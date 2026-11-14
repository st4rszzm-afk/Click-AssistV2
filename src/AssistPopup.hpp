#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Geode v5: Popup ya no es una plantilla
class AssistPopup : public Popup {
protected:
    CCNode* m_tabs[2] = {nullptr, nullptr};
    CCMenu* m_menus[2] = {nullptr, nullptr};
    ButtonSprite* m_tabSprites[2] = {nullptr, nullptr};
    CCLabelBMFont* m_windowLabel = nullptr;
    CCLabelBMFont* m_marginLabel = nullptr;
    Slider* m_windowSlider = nullptr;
    Slider* m_marginSlider = nullptr;

    static constexpr float WIN_MIN = 0.f,  WIN_MAX = 600.f;
    static constexpr float MAR_MIN = -5.f, MAR_MAX = 15.f;

    bool init() {
        if (!Popup::init(320.f, 320.f)) return false;
        this->setTitle("Click Assist");
        auto size = m_mainLayer->getContentSize();
        float cx = size.width / 2;

        auto icon = makeWaveIcon(0.9f);
        icon->setPosition({cx - 95.f, size.height - 22.f});
        m_mainLayer->addChild(icon);

        auto tabMenu = CCMenu::create();
        tabMenu->setPosition({0, 0});
        m_mainLayer->addChild(tabMenu);
        char const* names[2] = {"General", "Vehiculos"};
        for (int i = 0; i < 2; i++) {
            m_tabSprites[i] = ButtonSprite::create(
                names[i], 90, true, "bigFont.fnt", "GJ_button_04.png", 28.f, 0.6f);
            auto btn = CCMenuItemSpriteExtra::create(
                m_tabSprites[i], this, menu_selector(AssistPopup::onTab));
            btn->setTag(i);
            btn->setPosition({cx - 50.f + i * 100.f, size.height - 52.f});
            tabMenu->addChild(btn);

            m_tabs[i] = CCNode::create();
            m_mainLayer->addChild(m_tabs[i]);
            m_menus[i] = CCMenu::create();
            m_menus[i]->setPosition({0, 0});
            m_tabs[i]->addChild(m_menus[i]);
        }

        buildGeneral(size);
        buildVehicles(size);
        showTab(0);

        auto note = CCLabelBMFont::create("Solo para practicar. No para records.", "chatFont.fnt");
        note->setScale(0.6f);
        note->setOpacity(160);
        note->setPosition({cx, 16.f});
        m_mainLayer->addChild(note);
        return true;
    }

    void buildGeneral(CCSize size) {
        float cx = size.width / 2, top = size.height;
        auto mod = Mod::get();

        addToggle(0, "Activado",          "enabled", {35.f, top - 85.f});
        addToggle(0, "Dibujar hitboxes",  "draw",    {35.f, top - 113.f});
        addToggle(0, "Reconocer bloques", "blocks",  {35.f, top - 141.f});
        addToggle(0, "Reconocer rampas",  "slopes",  {35.f, top - 169.f});

        m_windowLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_windowLabel->setScale(0.55f);
        m_windowLabel->setPosition({cx, top - 198.f});
        m_tabs[0]->addChild(m_windowLabel);
        m_windowSlider = Slider::create(this, menu_selector(AssistPopup::onWindow), 0.8f);
        m_windowSlider->setPosition({cx, top - 216.f});
        m_tabs[0]->addChild(m_windowSlider);
        m_windowSlider->setValue(
            (mod->getSettingValue<int64_t>("window") - WIN_MIN) / (WIN_MAX - WIN_MIN));

        m_marginLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_marginLabel->setScale(0.55f);
        m_marginLabel->setPosition({cx, top - 244.f});
        m_tabs[0]->addChild(m_marginLabel);
        m_marginSlider = Slider::create(this, menu_selector(AssistPopup::onMargin), 0.8f);
        m_marginSlider->setPosition({cx, top - 262.f});
        m_tabs[0]->addChild(m_marginSlider);
        m_marginSlider->setValue(
            (mod->getSettingValue<double>("margin") - MAR_MIN) / (MAR_MAX - MAR_MIN));

        refreshLabels();
    }

    void buildVehicles(CCSize size) {
        float cx = size.width / 2, top = size.height;
        struct V { char const* name; char const* id; };
        V list[8] = {
            {"Cubo", "veh-cube"},   {"Nave", "veh-ship"},
            {"Bola", "veh-ball"},   {"UFO", "veh-ufo"},
            {"Wave", "veh-wave"},   {"Robot", "veh-robot"},
            {"Arana", "veh-spider"}, {"Swing", "veh-swing"},
        };
        for (int i = 0; i < 8; i++) {
            float x = (i % 2 == 0) ? 35.f : 175.f;
            float y = top - 90.f - (i / 2) * 30.f;
            addToggle(1, list[i].name, list[i].id, {x, y});
        }

        auto lbl = CCLabelBMFont::create("Intensidad", "goldFont.fnt");
        lbl->setScale(0.6f);
        lbl->setPosition({cx, top - 222.f});
        m_tabs[1]->addChild(lbl);

        char const* pn[3] = {"Suave", "Normal", "Estricto"};
        for (int i = 0; i < 3; i++) {
            auto spr = ButtonSprite::create(pn[i], 80, true, "bigFont.fnt",
                                            "GJ_button_01.png", 28.f, 0.55f);
            auto btn = CCMenuItemSpriteExtra::create(
                spr, this, menu_selector(AssistPopup::onPreset));
            btn->setTag(i);
            btn->setPosition({cx - 90.f + i * 90.f, top - 252.f});
            m_menus[1]->addChild(btn);
        }
    }

    void addToggle(int tab, char const* text, char const* settingId, CCPoint pos) {
        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(AssistPopup::onToggle), 0.65f);
        toggler->setPosition(pos);
        toggler->setUserObject("setting-id", CCString::create(settingId));
        toggler->toggle(Mod::get()->getSettingValue<bool>(settingId));
        m_menus[tab]->addChild(toggler);

        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setScale(0.42f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({pos.x + 22.f, pos.y});
        m_tabs[tab]->addChild(label);
    }

    void showTab(int i) {
        for (int k = 0; k < 2; k++) {
            m_tabs[k]->setVisible(k == i);
            m_tabSprites[k]->updateBGImage(k == i ? "GJ_button_01.png" : "GJ_button_04.png");
        }
    }

    void onTab(CCObject* sender) {
        showTab(static_cast<CCNode*>(sender)->getTag());
    }

    void onToggle(CCObject* sender) {
        auto t = static_cast<CCMenuItemToggler*>(sender);
        auto id = static_cast<CCString*>(t->getUserObject("setting-id"));
        // isToggled() devuelve el estado anterior al click
        Mod::get()->setSettingValue<bool>(id->getCString(), !t->isToggled());
    }

    void onPreset(CCObject* sender) {
        static constexpr int64_t W[3] = {150, 250, 400};
        static constexpr double  M[3] = {-2.0, 0.0, 6.0};
        int i = static_cast<CCNode*>(sender)->getTag();
        Mod::get()->setSettingValue<int64_t>("window", W[i]);
        Mod::get()->setSettingValue<double>("margin", M[i]);
        m_windowSlider->setValue((W[i] - WIN_MIN) / (WIN_MAX - WIN_MIN));
        m_marginSlider->setValue((M[i] - MAR_MIN) / (MAR_MAX - MAR_MIN));
        refreshLabels();
    }

    void onWindow(CCObject* sender) {
        float v = static_cast<SliderThumb*>(sender)->getValue();
        Mod::get()->setSettingValue<int64_t>(
            "window", static_cast<int64_t>(WIN_MIN + v * (WIN_MAX - WIN_MIN)));
        refreshLabels();
    }

    void onMargin(CCObject* sender) {
        float v = static_cast<SliderThumb*>(sender)->getValue();
        Mod::get()->setSettingValue<double>("margin", MAR_MIN + v * (MAR_MAX - MAR_MIN));
        refreshLabels();
    }

    void refreshLabels() {
        auto mod = Mod::get();
        m_windowLabel->setString(
            fmt::format("Ventana de correccion: {} ms", mod->getSettingValue<int64_t>("window")).c_str());
        m_marginLabel->setString(
            fmt::format("Margen de seguridad: {:.1f}", mod->getSettingValue<double>("margin")).c_str());
    }

public:
    static CCNode* makeWaveIcon(float scale) {
        auto gm = GameManager::get();
        auto player = SimplePlayer::create(gm->getPlayerDart());
        player->updatePlayerFrame(gm->getPlayerDart(), IconType::Wave);
        player->setColor(gm->colorForIdx(gm->getPlayerColor()));
        player->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
        player->setScale(scale);
        return player;
    }

    static AssistPopup* create() {
        auto ret = new AssistPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
