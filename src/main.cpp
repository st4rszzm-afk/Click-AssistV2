#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "Physics.hpp"
#include "AssistPopup.hpp"

using namespace geode::prelude;
using namespace assist;

class $modify(AssistLayer, GJBaseGameLayer) {
    struct Fields {
        std::vector<CCRect> hazards;
        std::vector<Solid> solids;
        bool cached = false;
        CCDrawNode* node = nullptr;
        bool userHolding = false;
        bool applied = false;
        float pending = -1.f;
    };

    void cacheLevel() {
        auto f = m_fields.self();
        f->hazards.clear(); f->solids.clear();
        f->cached = true;
        if (!m_objects) return;

        auto mod = Mod::get();
        bool useBlocks = mod->getSettingValue<bool>("blocks");
        bool useSlopes = mod->getSettingValue<bool>("slopes");

        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            switch (obj->m_objectType) {
                case GameObjectType::Hazard:
                    f->hazards.push_back(obj->getObjectRect()); break;
                case GameObjectType::Solid:
                    if (useBlocks) f->solids.push_back(makeSolid(obj)); break;
                case GameObjectType::Slope:
                    if (useSlopes) f->solids.push_back(makeSolid(obj)); break;
                default: break;
            }
        }
        std::sort(f->hazards.begin(), f->hazards.end(),
            [](CCRect const& a, CCRect const& b) { return a.getMinX() < b.getMinX(); });
        std::sort(f->solids.begin(), f->solids.end(),
            [](Solid const& a, Solid const& b) { return a.r.getMinX() < b.r.getMinX(); });
    }

    Mode currentMode() {
        auto p = m_player1;
        if (p->m_isShip)   return Mode::Ship;
        if (p->m_isBird)   return Mode::Ufo;
        if (p->m_isBall)   return Mode::Ball;
        if (p->m_isDart)   return Mode::Wave;
        if (p->m_isRobot)  return Mode::Robot;
        if (p->m_isSpider) return Mode::Spider;
        if (p->m_isSwing)  return Mode::Swing;
        return Mode::Cube;
    }

    bool vehicleOn(Mode m) {
        static char const* ids[] = {"veh-cube", "veh-ship", "veh-ball", "veh-ufo",
                                    "veh-wave", "veh-robot", "veh-spider", "veh-swing"};
        return Mod::get()->getSettingValue<bool>(ids[static_cast<int>(m)]);
    }

    Ctx makeCtx(int H) {
        auto f = m_fields.self();
        auto p = m_player1;
        Ctx c;
        auto r = p->getObjectRect();
        c.w = r.size.width; c.h = r.size.height;
        c.x = p->getPositionX(); c.y = p->getPositionY();
        c.vy = static_cast<float>(p->m_yVelocity);
        c.dir = p->m_isUpsideDown ? -1.f : 1.f;
        c.speed = unitsPerSec(p->m_playerSpeed) * TICK;
        c.grounded = p->m_isOnGround;
        c.mode = currentMode();

        float x0 = c.x - 80.f, x1 = c.x + c.speed * H + 80.f;

        auto hi = std::lower_bound(f->hazards.begin(), f->hazards.end(), x0 - 300.f,
            [](CCRect const& q, float v) { return q.getMinX() < v; });
        for (; hi != f->hazards.end() && hi->getMinX() <= x1; ++hi)
            if (hi->getMaxX() >= x0) c.hazards.push_back(*hi);

        auto si = std::lower_bound(f->solids.begin(), f->solids.end(), x0 - 300.f,
            [](Solid const& q, float v) { return q.r.getMinX() < v; });
        for (; si != f->solids.end() && si->r.getMinX() <= x1; ++si)
            if (si->r.getMaxX() >= x0) c.solids.push_back(*si);

        return c;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        auto mod = Mod::get();
        auto f = m_fields.self();

      if (!isPlayer1 || static_cast<GJBaseGameLayer*>(PlayLayer::get()) != static_cast<GJBaseGameLayer*>(this) || !mod->getSettingValue<bool>("enabled")
            || !vehicleOn(currentMode())) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        f->userHolding = down;
        Mode mode = currentMode();

        // Nave y wave: la decision se toma cada frame en update()
        if (mode == Mode::Ship || mode == Mode::Wave) return;

        if (down) {
            if (!f->cached) cacheLevel();
            const int H = 90;
            auto ctx = makeCtx(H);
            bool canAct = (mode == Mode::Ufo || mode == Mode::Swing) || ctx.grounded;
            float margin = static_cast<float>(mod->getSettingValue<double>("margin"));
            int maxD = static_cast<int>(mod->getSettingValue<int64_t>("window") * 0.06);

            if (canAct) {
                for (int d = 0; d <= maxD; d++) {
                    if (simulate(ctx, true, d, H, margin) == H) {
                        if (d > 0) { f->pending = d * TICK; return; }
                        break;
                    }
                }
            }
        }

        f->applied = down;
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void update(float dt) {
        auto mod = Mod::get();
        auto f = m_fields.self();

      if (static_cast<GJBaseGameLayer*>(PlayLayer::get()) != static_cast<GJBaseGameLayer*>(this) || !m_player1 || !mod->getSettingValue<bool>("enabled")) {
            return GJBaseGameLayer::update(dt);
        }
        if (!f->cached) cacheLevel();

        float margin = static_cast<float>(mod->getSettingValue<double>("margin"));
        Mode mode = currentMode();
        bool active = vehicleOn(mode);

        if (active && f->pending >= 0.f) {
            f->pending -= dt;
            if (f->pending <= 0.f) {
                f->pending = -1.f;
                GJBaseGameLayer::handleButton(true, 1, true);
                f->applied = true;
                if (!f->userHolding) {
                    GJBaseGameLayer::handleButton(false, 1, true);
                    f->applied = false;
                }
            }
        }

        if (active && (mode == Mode::Ship || mode == Mode::Wave)) {
            const int H = 40;
            auto ctx = makeCtx(H);
            bool want = f->userHolding;
            int sUser = simulate(ctx, want, 0, H, margin);
            if (sUser < H) {
                int sOther = simulate(ctx, !want, 0, H, margin);
                if (sOther > sUser) want = !want;
            }
            if (want != f->applied) {
                GJBaseGameLayer::handleButton(want, 1, true);
                f->applied = want;
            }
        } else if (active && f->pending < 0.f && f->applied != f->userHolding) {
            GJBaseGameLayer::handleButton(f->userHolding, 1, true);
            f->applied = f->userHolding;
        }

        GJBaseGameLayer::update(dt);

        if (mod->getSettingValue<bool>("draw")) drawDebug(margin);
        else if (f->node) f->node->clear();
    }

    void drawDebug(float margin) {
        auto f = m_fields.self();
        if (!f->node) {
            f->node = CCDrawNode::create();
            f->node->setZOrder(1000);
            m_objectLayer->addChild(f->node);
        }
        f->node->clear();

        const int H = 60;
        auto ctx = makeCtx(H);

        auto drawRect = [&](CCRect const& r, ccColor4F fill, ccColor4F border) {
            CCPoint v[4] = {
                {r.getMinX(), r.getMinY()}, {r.getMaxX(), r.getMinY()},
                {r.getMaxX(), r.getMaxY()}, {r.getMinX(), r.getMaxY()}
            };
            f->node->drawPolygon(v, 4, fill, 1.f, border);
        };

        for (auto const& r : ctx.hazards) drawRect(r, {1, 0, 0, 0.25f}, {1, 0, 0, 1});
        for (auto const& s : ctx.solids) {
            if (!s.slope) {
                drawRect(s.r, {0.2f, 0.4f, 1, 0.2f}, {0.2f, 0.4f, 1, 1});
            } else {
                CCPoint tri[3] = {s.a, s.b, s.corner};
                f->node->drawPolygon(tri, 3, {1, 0.6f, 0, 0.25f}, 1.f, {1, 0.6f, 0, 1});
            }
        }
        drawRect(m_player1->getObjectRect(), {0, 1, 0, 0.15f}, {0, 1, 0, 1});

        std::vector<CCPoint> path;
        bool act = (ctx.mode == Mode::Ship || ctx.mode == Mode::Wave) ? f->applied : true;
        simulate(ctx, act, 0, H, margin, &path);
        for (size_t i = 1; i < path.size(); i++)
            f->node->drawSegment(path[i - 1], path[i], 0.8f, {0, 1, 1, 1});
    }
};

class $modify(AssistMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto bg = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
        auto wave = AssistPopup::makeWaveIcon(0.8f);
        wave->setPosition(bg->getContentSize() / 2);
        bg->addChild(wave);

        auto btn = CCMenuItemSpriteExtra::create(
            bg, this, menu_selector(AssistMenuLayer::onAssist));
        btn->setID("click-assist-button"_spr);

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }

    void onAssist(CCObject*) { AssistPopup::create()->show(); }
};
