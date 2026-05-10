#include "ui/CloudSystem.h"
#include "world/Wind.h"
#include "world/World.h"
#include "core/Constants.h"
#include <cmath>
#include <random>

static constexpr float PI          = 3.14159265358979323846f;
static constexpr int   CLOUD_COUNT = 40;
static constexpr float WORLD_W     = World::WIDTH  * TILE_SIZE;
static constexpr float WORLD_H     = World::HEIGHT * TILE_SIZE;

CloudSystem::CloudSystem() {
    std::mt19937 rng(77);
    std::uniform_real_distribution<float> rx(0.0f, WORLD_W);
    std::uniform_real_distribution<float> ry(0.0f, WORLD_H);
    std::uniform_real_distribution<float> rrad(30.0f, 70.0f);
    std::uniform_real_distribution<float> rspd(22.0f, 50.0f);
    std::uniform_int_distribution<int>    ralpha(30, 65);

    clouds_.resize(CLOUD_COUNT);
    for (auto& c : clouds_) {
        c.wx     = rx(rng);
        c.wy     = ry(rng);
        c.radius = rrad(rng);
        c.speed  = rspd(rng);
        c.alpha  = (Uint8)ralpha(rng);
    }
}

void CloudSystem::update(float dt, const Wind& wind) {
    float rad = wind.direction() * PI / 180.0f;
    float wdx = std::sinf(rad);
    float wdy = -std::cosf(rad);

    for (auto& c : clouds_) {
        c.wx += wdx * c.speed * dt;
        c.wy += wdy * c.speed * dt;
        wrapCloud(c);
    }
}

void CloudSystem::wrapCloud(Cloud& c) {
    if (c.wx < 0)       c.wx += WORLD_W;
    if (c.wx > WORLD_W) c.wx -= WORLD_W;
    if (c.wy < 0)       c.wy += WORLD_H;
    if (c.wy > WORLD_H) c.wy -= WORLD_H;
}

void CloudSystem::fillCircle(SDL_Renderer* renderer, int cx, int cy, int r) {
    for (int dy = -r; dy <= r; ++dy) {
        int hw = (int)std::sqrtf((float)(r * r - dy * dy));
        SDL_RenderDrawLine(renderer, cx - hw, cy + dy, cx + hw, cy + dy);
    }
}

void CloudSystem::render(SDL_Renderer* renderer, float camX, float camY) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (const auto& c : clouds_) {
        int r  = (int)c.radius;
        int cx = (int)(c.wx - camX);
        int cy = (int)(c.wy - camY);

        // Skip clouds fully outside the viewport
        if (cx + r * 2 < 0 || cx - r * 2 > SCREEN_W) continue;
        if (cy + r * 2 < 0 || cy - r * 2 > SCREEN_H) continue;

        SDL_SetRenderDrawColor(renderer, 190, 195, 200, c.alpha);
        fillCircle(renderer, cx,                    cy,          r);
        fillCircle(renderer, cx + (int)(r * 0.9f), cy - r / 3, (int)(r * 0.75f));
        fillCircle(renderer, cx - (int)(r * 0.8f), cy - r / 4, (int)(r * 0.65f));
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
