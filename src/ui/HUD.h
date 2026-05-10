#pragma once
#include "ui/UIRenderer.h"
#include "ship/ShipStats.h"
#include "ship/EnemyShip.h"
#include "world/Wind.h"
#include "captain/GameTime.h"
#include <string>

class HUD {
public:
    explicit HUD(UIRenderer& ui);

    // Normal sailing HUD
    void render(const ShipStats& ship, const Wind& wind,
                float windFactor, int speedLevel, const GameTime& gameTime,
                int debugEnemyCount = -1);

    // Additional overlay drawn on top during combat
    void renderCombatOverlay(const EnemyShip& enemy, const ShipStats& player,
                              float distToEnemy, bool inBroadside);

private:
    UIRenderer& ui_;

    void drawWindCompass(const Wind& wind, int cx, int cy, int radius);
    void drawLine(int x1, int y1, int x2, int y2, SDL_Color color);
};
