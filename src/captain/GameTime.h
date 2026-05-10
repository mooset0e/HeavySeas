#pragma once
#include <string>

class GameTime {
public:
    void advanceDays(int days);

    int         day()          const { return day_; }
    int         year()         const { return day_ / 360; }
    std::string seasonName()   const;
    std::string displayString() const;

private:
    int day_ = 0;
};
