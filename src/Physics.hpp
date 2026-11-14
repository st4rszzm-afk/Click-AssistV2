#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace assist {
    constexpr float TICK = 1.f / 60.f;
    constexpr float GROUND_TOP = 90.f;
    constexpr float CEIL_TOP   = 390.f;
    constexpr float TOL_BLOCK  = 10.f;
    constexpr float TOL_SLOPE  = 14.f;

    // Constantes APROXIMADAS: calibralas comparando con la linea cian
    constexpr float JUMP = 11.18f, GRAV = 0.958f, TERMINAL = 15.f;
    constexpr float SHIP_UP = 0.6f, SHIP_DN = 0.5f, SHIP_MAX = 8.f;
    constexpr float BALL_G = 0.75f, BALL_MAX = 14.f;
    constexpr float UFO_JUMP = 7.5f, UFO_G = 0.45f, UFO_MAX = 8.f;
    constexpr float ROBOT_JUMP = 9.f;
    constexpr int   ROBOT_HOLD = 12;
    constexpr float SWING_G = 0.8f, SWING_MAX = 9.5f;

    // El orden debe coincidir con los ids "veh-*" de main.cpp
    enum class Mode { Cube, Ship, Ball, Ufo, Wave, Robot, Spider, Swing };

    struct Solid {
        CCRect r;
        bool slope = false;
        bool floorType = true;
        CCPoint a, b, corner;
    };

    struct Ctx {
        float x, y, vy, speed, w, h, dir;
        bool grounded;
        float rideVy = 0.f;
        int hold = 0;
        Mode mode;
        std::vector<CCRect> hazards;
        std::vector<Solid> solids;
    };

    inline float unitsPerSec(float m) {
        if (m < 0.8f) return 251.16f;
        if (m < 1.0f) return 311.58f;
        if (m < 1.2f) return 387.42f;
        if (m < 1.45f) return 468.0f;
        return 576.0f;
    }

    inline float surfaceAt(Solid const& s, float x) {
        float t = std::clamp((x - s.a.x) / std::max(1.f, s.b.x - s.a.x), 0.f, 1.f);
        return s.a.y + t * (s.b.y - s.a.y);
    }

    inline Solid makeSolid(GameObject* o) {
        Solid s;
        s.r = o->getObjectRect();
        if (o->m_objectType != GameObjectType::Slope) return s;

        s.slope = true;
        float cx = 0.5f, cy = -0.5f;
        if (o->isFlipX()) cx = -cx;
        if (o->isFlipY()) cy = -cy;
        int rot = ((int)std::lround(o->getRotation() / 90.f) % 4 + 4) % 4;
        for (int i = 0; i < rot; i++) { float t = cx; cx = cy; cy = -t; }

        float mx = s.r.getMidX(), my = s.r.getMidY();
        float w = s.r.size.width, h = s.r.size.height;
        s.corner = {mx + cx * w, my + cy * h};
        s.a = {mx - cx * w, my + cy * h};
        s.b = {mx + cx * w, my - cy * h};
        if (s.a.x > s.b.x) std::swap(s.a, s.b);
        s.floorType = cy < 0.f;
        return s;
    }

    inline bool slides(Mode m) {
        return m == Mode::Ship || m == Mode::Ufo || m == Mode::Ball ||
               m == Mode::Swing || m == Mode::Spider;
    }
    inline bool tunnel(Mode m) { return m != Mode::Cube && m != Mode::Robot; }

    inline bool collide(Ctx& c, float prevY) {
        float hh = c.h / 2, prevBot = prevY - hh, prevTop = prevY + hh;
        c.grounded = false;

        float lo = GROUND_TOP + hh;
        if (c.y < lo) {
            c.y = lo; if (c.vy < 0) c.vy = 0;
            if (c.dir > 0) c.grounded = true;
        }
        if (tunnel(c.mode) || c.dir < 0) {
            float hi = CEIL_TOP - hh;
            if (c.y > hi) {
                c.y = hi; if (c.vy > 0) c.vy = 0;
                if (c.dir < 0) c.grounded = true;
            }
        }

        for (auto const& s : c.solids) {
            CCRect pr(c.x - c.w / 2 + 0.5f, c.y - hh + 0.5f, c.w - 1.f, c.h - 1.f);
            if (!pr.intersectsRect(s.r)) continue;

            if (!s.slope) {
                if (c.mode == Mode::Wave) return true;
                bool above = prevBot >= s.r.getMaxY() - TOL_BLOCK;
                bool below = prevTop <= s.r.getMinY() + TOL_BLOCK;
                if (above) {
                    if (c.dir > 0 || slides(c.mode)) {
                        c.y = s.r.getMaxY() + hh; if (c.vy < 0) c.vy = 0;
                        if (c.dir > 0) c.grounded = true;
                    } else return true;
                } else if (below) {
                    if (c.dir < 0 || slides(c.mode)) {
                        c.y = s.r.getMinY() - hh; if (c.vy > 0) c.vy = 0;
                        if (c.dir < 0) c.grounded = true;
                    } else return true;
                } else return true;
            } else {
                if (c.mode != Mode::Wave && s.floorType != (c.dir > 0)) continue;

                float sg = s.floorType ? 1.f : -1.f;
                float surf = surfaceAt(s, c.x);
                float edge = c.y - sg * hh;
                float prevEdge = prevY - sg * hh;
                float base = s.floorType ? s.r.getMinY() : s.r.getMaxY();
                bool asc = sg * (s.b.y - s.a.y) > 0.f;

                if (sg * (edge - surf) < 0.f) {
                    bool onTop = sg * (prevEdge - surf) >= -TOL_SLOPE;
                    bool climbing = asc && sg * (prevEdge - base) >= -TOL_BLOCK;
                    if (!(onTop || climbing)) return true;
                    c.y = surf + sg * hh;
                    c.vy = 0.f;
                    c.grounded = true;
                    float rate = (s.b.y - s.a.y) / std::max(1.f, s.b.x - s.a.x);
                    c.rideVy = rate * c.speed;
                }
            }
        }
        return false;
    }

    inline void spiderTeleport(Ctx& c) {
        float hh = c.h / 2, l = c.x - c.w / 2, r = c.x + c.w / 2;
        if (c.dir > 0) {
            float best = CEIL_TOP;
            for (auto const& s : c.solids)
                if (s.r.getMinX() < r && s.r.getMaxX() > l && s.r.getMinY() >= c.y + hh - 1.f)
                    best = std::min(best, s.r.getMinY());
            c.y = best - hh;
        } else {
            float best = GROUND_TOP;
            for (auto const& s : c.solids)
                if (s.r.getMinX() < r && s.r.getMaxX() > l && s.r.getMaxY() <= c.y - hh + 1.f)
                    best = std::max(best, s.r.getMaxY());
            c.y = best + hh;
        }
        c.dir = -c.dir; c.vy = 0.f; c.grounded = true;
    }

    inline void step(Ctx& c, bool act, bool click) {
        float d = c.dir;
        switch (c.mode) {
            case Mode::Cube:
                if (click && c.grounded) { c.vy = JUMP * d; c.grounded = false; }
                c.vy = std::clamp(c.vy - GRAV * d, -TERMINAL, TERMINAL);
                c.y += c.vy;
                break;
            case Mode::Ball:
                if (click && c.grounded) { c.dir = -c.dir; d = c.dir; c.vy = 0.f; c.grounded = false; }
                c.vy = std::clamp(c.vy - BALL_G * d, -BALL_MAX, BALL_MAX);
                c.y += c.vy;
                break;
            case Mode::Ufo:
                if (click) c.vy = UFO_JUMP * d;
                c.vy = std::clamp(c.vy - UFO_G * d, -UFO_MAX, UFO_MAX);
                c.y += c.vy;
                break;
            case Mode::Robot: {
                if (click && c.grounded) { c.vy = ROBOT_JUMP * d; c.hold = ROBOT_HOLD; c.grounded = false; }
                float g = GRAV;
                if (c.hold > 0 && act) { c.hold--; g *= 0.15f; } else c.hold = 0;
                c.vy = std::clamp(c.vy - g * d, -TERMINAL, TERMINAL);
                c.y += c.vy;
                break;
            }
            case Mode::Spider:
                if (click && c.grounded) { spiderTeleport(c); break; }
                c.vy = std::clamp(c.vy - GRAV * d, -TERMINAL, TERMINAL);
                c.y += c.vy;
                break;
            case Mode::Swing:
                if (click) { c.dir = -c.dir; d = c.dir; c.vy *= 0.4f; }
                c.vy = std::clamp(c.vy - SWING_G * d, -SWING_MAX, SWING_MAX);
                c.y += c.vy;
                break;
            case Mode::Ship:
                c.vy = std::clamp(c.vy + (act ? SHIP_UP : -SHIP_DN) * d, -SHIP_MAX, SHIP_MAX);
                c.y += c.vy;
                break;
            case Mode::Wave:
                c.vy = (act ? 1.f : -1.f) * d * c.speed;
                c.y += c.vy;
                break;
        }
    }

    inline int simulate(Ctx c, bool act, int delay, int H, float margin,
                        std::vector<CCPoint>* path = nullptr) {
        bool discrete = c.mode != Mode::Ship && c.mode != Mode::Wave;
        for (int t = 0; t < H; t++) {
            bool click = discrete && act && t == delay;
            float prevY = c.y;
            bool wasGrounded = c.grounded;
            float launch = c.rideVy;
            c.rideVy = 0.f;

            c.x += c.speed;
            step(c, discrete ? (act && t >= delay) : act, click);
            if (collide(c, prevY)) { if (path) path->push_back({c.x, c.y}); return t; }

            if (wasGrounded && !c.grounded && launch > 0.f && !click) c.vy = launch * c.dir;

            if (path) path->push_back({c.x, c.y});

            CCRect pr(c.x - c.w / 2 - margin, c.y - c.h / 2 - margin,
                      c.w + margin * 2, c.h + margin * 2);
            for (auto const& hz : c.hazards) if (pr.intersectsRect(hz)) return t;
        }
        return H;
    }
}
