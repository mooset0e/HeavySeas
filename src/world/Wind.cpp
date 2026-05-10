#include "world/Wind.h"
#include <cmath>
#include <random>

static std::mt19937 rng(99);

Wind::Wind() {
    std::uniform_real_distribution<float> dir(0.0f, 360.0f);
    direction_ = targetDirection_ = dir(rng);
    changeTimer_ = 20.0f;
}

void Wind::update(float dt) {
    changeTimer_ -= dt;
    if (changeTimer_ <= 0.0f) {
        std::uniform_real_distribution<float> shift(-60.0f, 60.0f);
        std::uniform_real_distribution<float> wait(15.0f, 35.0f);
        targetDirection_ = std::fmod(targetDirection_ + shift(rng) + 360.0f, 360.0f);
        changeTimer_ = wait(rng);
    }

    // Smoothly rotate toward target (shortest arc)
    float diff = targetDirection_ - direction_;
    if (diff >  180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    direction_ = std::fmod(direction_ + diff * dt * 0.4f + 360.0f, 360.0f);
}

float Wind::speedFactor(float shipHeadingDeg) const {
    float diff = std::abs(shipHeadingDeg - direction_);
    if (diff > 180.0f) diff = 360.0f - diff;
    // tailwind (0°) = 1.0, headwind (180°) = 0.3
    return 1.0f - 0.7f * (diff / 180.0f);
}

std::string Wind::directionName() const {
    static const char* names[] = { "N","NE","E","SE","S","SW","W","NW" };
    int idx = (int)((direction_ + 22.5f) / 45.0f) % 8;
    return names[idx];
}
