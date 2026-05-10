#include "captain/GameTime.h"

void GameTime::advanceDays(int days) { day_ += days; }

std::string GameTime::seasonName() const {
    int d = day_ % 360;
    if (d < 90)  return "Spring";
    if (d < 180) return "Summer";
    if (d < 270) return "Autumn";
    return "Winter";
}

std::string GameTime::displayString() const {
    return "Year " + std::to_string(year() + 1) + ", " + seasonName();
}
