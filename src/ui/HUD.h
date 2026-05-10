#pragma once
#include "ui/UIRenderer.h"
#include "ship/ShipStats.h"
#include "world/Wind.h"
#include <string>

class HUD {
public:
    explicit HUD(UIRenderer& ui);

    void render(const ShipStats& ship, const Wind& wind, float speedFactor);

private:
    UIRenderer& ui_;

    void drawWindCompass(const Wind& wind, int cx, int cy, int radius);
    void drawLine(int x1, int y1, int x2, int y2, SDL_Color color);
};
