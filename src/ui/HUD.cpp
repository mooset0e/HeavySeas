#include "ui/HUD.h"
#include "ship/ShipType.h"
#include "core/Constants.h"
#include <cmath>
#include <string>

static constexpr float PI = 3.14159265358979323846f;

HUD::HUD(UIRenderer& ui) : ui_(ui) {}

void HUD::drawLine(int x1, int y1, int x2, int y2, SDL_Color c) {
    SDL_SetRenderDrawColor(ui_.renderer(), c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(ui_.renderer(), x1, y1, x2, y2);
}

void HUD::drawWindCompass(const Wind& wind, int cx, int cy, int radius) {
    SDL_Color ring   = { 70, 120, 155, 255 };
    SDL_Color dimmed = { 50,  80, 100, 255 };
    static const char* cardinals[] = { "N", "E", "S", "W" };
    float cardAngles[] = { 0, 90, 180, 270 };
    for (int i = 0; i < 4; ++i) {
        float a = cardAngles[i] * PI / 180.0f;
        int tx = cx + (int)(sinf(a) * (radius + 12));
        int ty = cy - (int)(cosf(a) * (radius + 12));
        int ix = cx + (int)(sinf(a) * (radius - 4));
        int iy = cy - (int)(cosf(a) * (radius - 4));
        int ox = cx + (int)(sinf(a) * radius);
        int oy = cy - (int)(cosf(a) * radius);
        drawLine(ix, iy, ox, oy, ring);
        ui_.drawTextCentered(cardinals[i], { tx - 6, ty - 8, 12, 16 }, dimmed);
    }

    float rad = wind.direction() * PI / 180.0f;
    int tipX = cx + (int)(sinf(rad) * (radius - 2));
    int tipY = cy - (int)(cosf(rad) * (radius - 2));

    SDL_Color arrow = { 255, 220, 80, 255 };
    drawLine(cx, cy, tipX, tipY, arrow);

    float wing = 0.45f;
    float wlen = radius * 0.35f;
    drawLine(tipX, tipY,
             tipX - (int)(sinf(rad + wing) * wlen),
             tipY + (int)(cosf(rad + wing) * wlen), arrow);
    drawLine(tipX, tipY,
             tipX - (int)(sinf(rad - wing) * wlen),
             tipY + (int)(cosf(rad - wing) * wlen), arrow);
}

void HUD::render(const ShipStats& ship, const Wind& wind,
                  float windFactor, int speedLevel, const GameTime& gameTime,
                  int debugEnemyCount)
{
    // --- Bottom-left: ship stats ---
    SDL_Rect panel{ 10, SCREEN_H - 180, 310, 170 };
    ui_.drawPanel(panel, { 10, 20, 45, 210 }, { 70, 120, 155, 255 }, 2);

    SDL_Color gold = { 255, 220,  80, 255 };
    SDL_Color text = { 200, 180, 140, 255 };

    ui_.drawText(ship.name,                                                              panel.x + 10, panel.y + 10, gold);
    ui_.drawText("Gold:  " + std::to_string(ship.gold),                                 panel.x + 10, panel.y + 38, text);
    ui_.drawText("Crew:  " + std::to_string(ship.crewCur) + "/" + std::to_string(ship.crewMax),
                                                                                         panel.x + 10, panel.y + 60, text);
    ui_.drawText("Cargo: " + std::to_string(ship.cargoCur) + "/" + std::to_string(ship.cargoMax),
                                                                                         panel.x + 10, panel.y + 82, text);
    ui_.drawText("Hull:",                                                                panel.x + 10, panel.y + 106, text);

    SDL_Color hullCol = ship.hullCur > ship.hullMax / 2
        ? SDL_Color{ 70, 200, 70, 255 } : SDL_Color{ 220, 80, 60, 255 };
    SDL_Rect hullBar{ panel.x + 65, panel.y + 108, 230, 14 };
    ui_.drawHealthBar(ship.hullCur, ship.hullMax, hullBar, hullCol);

    static const char* speedLabels[]  = { "ANCHORED", "SLOW", "MEDIUM", "FULL SAIL" };
    static const SDL_Color speedCols[] = {
        { 120, 120, 120, 255 },
        {  80, 160, 220, 255 },
        { 100, 210, 120, 255 },
        { 255, 220,  80, 255 },
    };
    ui_.drawText("Sails:", panel.x + 10, panel.y + 132, text);
    ui_.drawText(speedLabels[speedLevel], panel.x + 70, panel.y + 132, speedCols[speedLevel]);

    for (int i = 1; i <= 3; ++i) {
        SDL_Color pip = (i <= speedLevel) ? speedCols[speedLevel] : SDL_Color{ 50, 60, 80, 255 };
        SDL_Rect  dot{ panel.x + 200 + (i - 1) * 22, panel.y + 135, 12, 12 };
        SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(ui_.renderer(), pip.r, pip.g, pip.b, pip.a);
        SDL_RenderFillRect(ui_.renderer(), &dot);
    }

    // --- Top-right: wind compass ---
    int cx = SCREEN_W - 75, cy = 80, radius = 38;
    SDL_Rect compassBg{ cx - radius - 18, cy - radius - 18,
                        (radius + 18) * 2,  (radius + 18) * 2 };
    ui_.drawPanel(compassBg, { 10, 20, 45, 200 }, { 70, 120, 155, 255 }, 2);
    drawWindCompass(wind, cx, cy, radius);

    std::string windLabel = "Wind: " + wind.directionName();
    ui_.drawTextCentered(windLabel, { cx - 50, cy + radius + 8, 100, 20 }, { 200, 180, 140, 255 });

    SDL_Rect speedBarBg{ cx - 50, cy + radius + 32, 100, 10 };
    ui_.drawPanel(speedBarBg, { 10, 20, 45, 200 }, { 70, 120, 155, 255 }, 1);
    SDL_Color speedCol = windFactor > 0.7f
        ? SDL_Color{ 70, 200, 70, 255 }
        : windFactor > 0.45f
            ? SDL_Color{ 220, 180, 50, 255 }
            : SDL_Color{ 220, 80, 60, 255 };
    ui_.drawHealthBar((int)(windFactor * 100), 100, speedBarBg, speedCol);
    ui_.drawTextCentered("Speed", { cx - 50, cy + radius + 44, 100, 16 }, { 120, 120, 120, 255 });

    // --- Top-left: game time + debug ---
    std::string timeStr = gameTime.displayString();
    if (debugEnemyCount >= 0)
        timeStr += "   [ships:" + std::to_string(debugEnemyCount) + "]";
    ui_.drawText(timeStr, 12, 10, { 150, 160, 140, 255 });

    // --- Reload bar if in combat (reloadTimer > 0) ---
    if (ship.reloadTimer > 0.0f) {
        auto cannon    = getCannonConfig(ship.cannonTier);
        float fraction = ship.reloadTimer / cannon.reload;   // 1 = just fired, 0 = ready
        SDL_Rect reloadBg{ panel.x, panel.y - 22, panel.w, 14 };
        ui_.drawPanel(reloadBg, { 10, 10, 10, 200 }, { 70, 120, 155, 255 }, 1);
        SDL_Rect reloadFill{ reloadBg.x + 2, reloadBg.y + 2,
                             (int)((reloadBg.w - 4) * (1.0f - fraction)), reloadBg.h - 4 };
        SDL_SetRenderDrawColor(ui_.renderer(), 255, 140, 30, 255);
        SDL_RenderFillRect(ui_.renderer(), &reloadFill);
        ui_.drawTextCentered("RELOADING", reloadBg, { 200, 130, 40, 255 });
    } else if (ship.reloadTimer == 0.0f) {
        // Show "READY" hint only during combat (HUD doesn't know mode, so just skip)
    }
}

void HUD::renderCombatOverlay(const EnemyShip& enemy, const ShipStats& player,
                               float distToEnemy, bool inBroadside)
{
    // --- Top-center: enemy health ---
    const char* cls = "";
    switch (enemy.shipClass) {
        case ShipClass::Sloop:      cls = "Sloop";      break;
        case ShipClass::Brigantine: cls = "Brigantine"; break;
        case ShipClass::Frigate:    cls = "Frigate";    break;
        case ShipClass::Galleon:    cls = "Galleon";    break;
        case ShipClass::ManOWar:    cls = "Man-o'-War"; break;
    }
    std::string label = std::string(cls) + "  Hull: " +
                        std::to_string(enemy.hullCur) + "/" + std::to_string(enemy.hullMax);

    SDL_Rect enemyPanel{ SCREEN_W / 2 - 180, 8, 360, 46 };
    ui_.drawPanel(enemyPanel, { 40, 8, 8, 220 }, { 200, 70, 50, 255 }, 2);
    ui_.drawTextCentered(label, { enemyPanel.x, enemyPanel.y + 4, enemyPanel.w, 18 },
                          { 255, 200, 180, 255 });

    SDL_Color hullCol = enemy.hullCur > enemy.hullMax / 2
        ? SDL_Color{ 220, 120, 50, 255 } : SDL_Color{ 200, 50, 50, 255 };
    SDL_Rect enemyHull{ enemyPanel.x + 8, enemyPanel.y + 26, enemyPanel.w - 16, 12 };
    ui_.drawHealthBar(enemy.hullCur, enemy.hullMax, enemyHull, hullCol);

    // --- Broadside indicator ---
    if (inBroadside) {
        SDL_Color ready = { 255, 220, 50, 255 };
        ui_.drawTextCentered("[SPACE] BROADSIDE!",
                              { SCREEN_W / 2 - 140, 64, 280, 24 }, ready);
    } else if (player.reloadTimer <= 0.0f) {
        ui_.drawTextCentered("Maneuver to broadside",
                              { SCREEN_W / 2 - 140, 64, 280, 24 }, { 120, 120, 120, 255 });
    }

    // --- Crew / morale strip ---
    std::string crewStr = "Enemy crew: " + std::to_string(enemy.crewCur) +
                          "    Morale: " + std::to_string((int)enemy.morale) + "%";
    ui_.drawText(crewStr, SCREEN_W / 2 - 140, 96, { 160, 140, 120, 255 });
}
