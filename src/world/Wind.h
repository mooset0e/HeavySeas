#pragma once
#include <string>

class Wind {
public:
    Wind();
    void update(float dt);

    float direction() const { return direction_; }
    float speedFactor(float shipHeadingDeg) const;
    std::string directionName() const;

private:
    float direction_       = 0.0f;
    float targetDirection_ = 0.0f;
    float changeTimer_     = 0.0f;
};
