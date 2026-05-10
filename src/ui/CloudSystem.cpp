#include "ui/CloudSystem.h"
#include "world/Wind.h"
#include "core/Constants.h"
#include <cmath>
#include <random>

static constexpr float PI = 3.14159265358979323846f;
static constexpr int   CLOUD_COUNT = 12;

CloudSystem::CloudSystem() {
    std::mt19937 rng(77);
    std::uniform_real_distribution<float> rx(0.0f, (float)SCREEN_W);
    std::uniform_real_distribution<float> ry(0.0f, (float)SCREEN_H);
    std::uniform_real_distribution<float> rrad(24.0f, 52.0f);
    std::uniform_real_distribution<float> rspd(18.0f, 38.0f);
    std::uniform_int_distribution<int>    ralpha(35, 70);

    clouds_.resize(CLOUD_COUNT);
    for (auto& c : clouds_) {
        c.x      = rx(rng);
        c.y      = ry(rng);
        c.radius = rrad(rng);
        c.speed  = rspd(rng);
        c.alpha  = (Uint8)ralpha(rng);
    }
}

void CloudSystem::update(float dt, const Wind& wind) {
    float rad  = wind.direction() * PI / 180.0f;
    float wdx  = std::sinf(rad);
    float wdy  = -std::cosf(rad);

    for (auto& c : clouds_) {
        c.x += wdx * c.speed * dt;
        c.y += wdy * c.speed * dt;
        wrapCloud(c, wdx, wdy);
    }
}

void CloudSystem::wrapCloud(Cloud& c, float wdx, float wdy) {
    float margin = c.radius * 2.0f;
    if (wdx > 0 && c.x - margin > SCREEN_W)  c.x = -margin;
    if (wdx < 0 && c.x + margin < 0)          c.x = SCREEN_W + margin;
    if (wdy > 0 && c.y - margin > SCREEN_H)   c.y = -margin;
    if (wdy < 0 && c.y + margin < 0)           c.y = SCREEN_H + margin;
}

void CloudSystem::fillCircle(SDL_Renderer* renderer, int cx, int cy, int r) {
    for (int dy = -r; dy <= r; ++dy) {
        int hw = (int)std::sqrtf((float)(r * r - dy * dy));
        SDL_RenderDrawLine(renderer, cx - hw, cy + dy, cx + hw, cy + dy);
    }
}

void CloudSystem::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (const auto& c : clouds_) {
        int r = (int)c.radius;
        int cx = (int)c.x;
        int cy = (int)c.y;

        // Each cloud is 3 overlapping circles — left lobe, center, right lobe
        SDL_SetRenderDrawColor(renderer, 190, 195, 200, c.alpha);
        fillCircle(renderer, cx,             cy,          r);
        fillCircle(renderer, cx + (int)(r * 0.9f), cy - r / 3, (int)(r * 0.75f));
        fillCircle(renderer, cx - (int)(r * 0.8f), cy - r / 4, (int)(r * 0.65f));
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
