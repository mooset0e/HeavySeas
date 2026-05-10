#include "ship/EnemyManager.h"
#include "ship/ShipType.h"
#include "core/Constants.h"
#include <cmath>
#include <algorithm>

float EnemyManager::dist(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by;
    return std::sqrtf(dx * dx + dy * dy);
}

void EnemyManager::steerToward(EnemyShip& e, float tx, float ty, float dt) {
    float dx = tx - e.x, dy = ty - e.y;
    if (std::fabsf(dx) < 0.01f && std::fabsf(dy) < 0.01f) return;
    float target = std::atan2f(dx, -dy) * 180.0f / PI;
    if (target < 0.0f) target += 360.0f;
    float diff = target - e.heading;
    if (diff >  180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    float maxTurn = getShipTypeDef(e.shipClass).turnRate * dt;
    e.heading = std::fmodf(e.heading + std::max(-maxTurn, std::min(diff, maxTurn)) + 360.0f, 360.0f);
}

void EnemyManager::moveShip(EnemyShip& e, float dt, const Wind& wind) {
    float speed = getShipTypeDef(e.shipClass).speed * wind.speedFactor(e.heading);
    float rad   = e.heading * PI / 180.0f;
    e.x += std::sinf(rad) * speed * dt;
    e.y -= std::cosf(rad) * speed * dt;
    e.x = std::max(0.5f, std::min(e.x, (float)128 - 0.5f));
    e.y = std::max(0.5f, std::min(e.y, (float)72  - 0.5f));
}

void EnemyManager::addPlotShip(EnemyShip ship) {
    ship.isVisible = true;
    ships_.push_back(ship);
}

void EnemyManager::update(float dt, float px, float py, const Wind& wind) {
    trigger_ = -1;
    for (int i = 0; i < (int)ships_.size(); ++i) {
        EnemyShip& e = ships_[i];
        if (!e.alive()) continue;

        float d = dist(e.x, e.y, px, py);

        if (e.state == AIState::Patrolling) {
            if (d < 10.0f) e.state = AIState::Chasing;
            else {
                steerToward(e, e.patrol.cx, e.patrol.cy, dt);
                if (dist(e.x, e.y, e.patrol.cx, e.patrol.cy) < 1.5f) {
                    // Simple back-and-forth: mirror the patrol centre
                    e.patrol.cx = e.destX;
                    e.patrol.cy = e.destY;
                }
            }
        } else if (e.state == AIState::Chasing) {
            steerToward(e, px, py, dt);
            if (d < ENCOUNTER_RANGE && trigger_ == -1)
                trigger_ = i;
        } else if (e.state == AIState::Fleeing) {
            // Flee away from player
            steerToward(e, 2.0f * e.x - px, 2.0f * e.y - py, dt);
        }

        moveShip(e, dt, wind);
    }
}

void EnemyManager::render(SDL_Renderer* renderer, float camX, float camY) const {
    for (const auto& e : ships_) {
        if (!e.alive() || !e.isVisible) continue;

        int esx = (int)(e.x * TILE_SIZE - camX);
        int esy = (int)(e.y * TILE_SIZE - camY);
        if (esx < -TILE_SIZE || esx > 1280 + TILE_SIZE ||
            esy < -TILE_SIZE || esy > 720  + TILE_SIZE)
            continue;

        // Colour by faction
        SDL_Color col;
        switch (e.faction) {
            case Faction::Pirate:   col = { 220,  50,  50, 255 }; break;
            case Faction::Navy:     col = {  50,  80, 200, 255 }; break;
            case Faction::Merchant: col = { 220, 180,  50, 255 }; break;
        }

        float hRad = e.heading * PI / 180.0f;
        int   sz   = TILE_SIZE / 3;
        int fx  = esx + (int)(std::sinf(hRad) * sz);
        int fy  = esy - (int)(std::cosf(hRad) * sz);
        int blx = esx + (int)(std::sinf(hRad + 2.4f) * sz * 0.65f);
        int bly = esy - (int)(std::cosf(hRad + 2.4f) * sz * 0.65f);
        int brx = esx + (int)(std::sinf(hRad - 2.4f) * sz * 0.65f);
        int bry = esy - (int)(std::cosf(hRad - 2.4f) * sz * 0.65f);

        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
        for (int t = 0; t <= 20; ++t) {
            float tf = t / 20.0f;
            SDL_RenderDrawLine(renderer, fx, fy,
                blx + (int)((brx - blx) * tf),
                bly + (int)((bry - bly) * tf));
        }

        // Glowing ring to distinguish mission ships
        SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
        int r = sz + 8;
        for (int a = 0; a < 36; a += 2) {
            float a0 = a       * PI / 18.0f;
            float a1 = (a + 1) * PI / 18.0f;
            SDL_RenderDrawLine(renderer,
                esx + (int)(std::cosf(a0) * r), esy + (int)(std::sinf(a0) * r),
                esx + (int)(std::cosf(a1) * r), esy + (int)(std::sinf(a1) * r));
        }
    }
}

int EnemyManager::checkEncounterTrigger(float px, float py) {
    if (trigger_ >= 0) {
        int idx  = trigger_;
        trigger_ = -1;
        return idx;
    }
    return -1;
}
