#include "ui/HUD.h"
#include "core/Constants.h"

HUD::HUD(UIRenderer& ui) : ui_(ui) {}

void HUD::render(const ShipStats& ship) {
    SDL_Rect panel{ 10, SCREEN_H - 160, 310, 150 };
    ui_.drawPanel(panel, { 10, 20, 45, 210 }, { 70, 120, 155, 255 }, 2);

    SDL_Color gold  = { 255, 220, 80,  255 };
    SDL_Color text  = { 200, 180, 140, 255 };

    ui_.drawText(ship.name,                                              panel.x + 10, panel.y + 10,  gold);
    ui_.drawText("Gold:  " + std::to_string(ship.gold),                 panel.x + 10, panel.y + 38,  text);
    ui_.drawText("Crew:  " + std::to_string(ship.crewCur) + "/" + std::to_string(ship.crewMax),
                                                                         panel.x + 10, panel.y + 60,  text);
    ui_.drawText("Cargo: " + std::to_string(ship.cargoCur) + "/" + std::to_string(ship.cargoMax),
                                                                         panel.x + 10, panel.y + 82,  text);
    ui_.drawText("Hull:",                                                panel.x + 10, panel.y + 106, text);

    SDL_Color hullColor = ship.hullCur > ship.hullMax / 2
        ? SDL_Color{ 70, 200, 70, 255 }
        : SDL_Color{ 220, 80, 60, 255 };
    SDL_Rect hullBar{ panel.x + 65, panel.y + 108, 230, 14 };
    ui_.drawHealthBar(ship.hullCur, ship.hullMax, hullBar, hullColor);
}

void HUD::renderPortPrompt(const std::string& townName) {
    std::string msg = "[Enter]  Enter " + townName;
    SDL_Rect bounds{ SCREEN_W / 2 - 210, SCREEN_H - 52, 420, 42 };
    ui_.drawPanel(bounds, { 10, 20, 45, 210 }, { 70, 120, 155, 255 }, 2);
    ui_.drawTextCentered(msg, bounds, { 255, 220, 80, 255 });
}
