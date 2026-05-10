#pragma once
#include "ui/UIRenderer.h"
#include "ship/ShipStats.h"
#include <string>

class HUD {
public:
    explicit HUD(UIRenderer& ui);

    void render(const ShipStats& ship);
    void renderPortPrompt(const std::string& townName);

private:
    UIRenderer& ui_;
};
