#include "ui/EncounterScreen.h"
#include "core/InputState.h"
#include "core/Constants.h"
#include "core/Nation.h"

static const SDL_Color COL_BG     = {  8, 16, 38, 245 };
static const SDL_Color COL_BORDER = { 200, 70, 50, 255 };
static const SDL_Color COL_TITLE  = { 255, 180, 60, 255 };
static const SDL_Color COL_TEXT   = { 200, 180, 140, 255 };
static const SDL_Color COL_DIM    = {  90, 100, 100, 255 };
static const SDL_Color COL_WARN   = { 240, 100,  60, 255 };

static const char* factionLabel(Faction f) {
    switch (f) {
        case Faction::Navy:     return "Naval Vessel";
        case Faction::Merchant: return "Merchant Ship";
        case Faction::Pirate:   return "Pirate";
    }
    return "Unknown";
}

static const char* classLabel(ShipClass c) {
    switch (c) {
        case ShipClass::Sloop:      return "Sloop";
        case ShipClass::Brigantine: return "Brigantine";
        case ShipClass::Frigate:    return "Frigate";
        case ShipClass::Galleon:    return "Galleon";
        case ShipClass::ManOWar:    return "Man-o'-War";
    }
    return "Unknown";
}

EncounterScreen::EncounterScreen(UIRenderer& ui, const EnemyShip& enemy,
                                  const ShipStats& player, const Captain& captain)
    : ui_(ui), enemy_(enemy), player_(player), captain_(captain)
{
    bool identified = captain.hasLookout || captain.hasSpotter;
    bool isMerchant = identified && (enemy.faction == Faction::Merchant);
    bool isNavy     = identified && (enemy.faction == Faction::Navy);

    std::vector<MenuItem> items;
    items.push_back({ "Engage",      "Open fire — battle begins at once." });

    if (isMerchant) {
        items.push_back({ "Hail & Trade",  "Flag them down for a peaceful exchange." });
        items.push_back({ "Rob & Plunder", "Demand their cargo by force of reputation." });
    } else if (isNavy) {
        items.push_back({ "Hail",          "Show your papers and sail peacefully." });
    } else {
        items.push_back({ "Hail",          "Signal the unknown vessel." });
    }
    items.push_back({ "Flee",         "Turn and run — they may still catch you." });

    menu_ = Menu(items);

    // Parallel choice list maps each menu item → EncounterChoice
    itemChoices_.push_back(EncounterChoice::Engage);
    if (isMerchant) {
        itemChoices_.push_back(EncounterChoice::Hail);
        itemChoices_.push_back(EncounterChoice::Rob);
    } else {
        itemChoices_.push_back(EncounterChoice::Hail);
    }
    itemChoices_.push_back(EncounterChoice::Flee);
}

void EncounterScreen::handleInput(const InputState& input) {
    if (!resultMsg.empty()) {
        // Any key dismisses the result
        if (input.justPressed(SDL_SCANCODE_RETURN) ||
            input.justPressed(SDL_SCANCODE_SPACE)  ||
            input.justPressed(SDL_SCANCODE_ESCAPE))
            choice_ = EncounterChoice::Flee;   // result displayed; close to sailing
        return;
    }
    if (input.justPressed(SDL_SCANCODE_UP))   menu_.moveUp();
    if (input.justPressed(SDL_SCANCODE_DOWN)) menu_.moveDown();
    if (input.justPressed(SDL_SCANCODE_RETURN)) {
        int idx = menu_.selectedIndex();
        if (idx >= 0 && idx < (int)itemChoices_.size())
            choice_ = itemChoices_[idx];
    }
}

void EncounterScreen::render() {
    SDL_Rect panel{ PX, PY, PW, PH };

    // Background dim
    SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ui_.renderer(), 0, 0, 0, 160);
    SDL_Rect fs{ 0, 0, SCREEN_W, SCREEN_H };
    SDL_RenderFillRect(ui_.renderer(), &fs);

    ui_.drawPanel(panel, COL_BG, COL_BORDER, 2);

    // Title bar
    SDL_Rect titleR{ PX, PY, PW, 52 };
    ui_.drawPanel(titleR, { 50, 10, 10, 255 }, COL_BORDER, 2);
    ui_.drawTextCentered("! SHIP SIGHTED !", titleR, COL_TITLE);

    // If showing a deferred result (Hail/Rob outcome), display it instead
    if (!resultMsg.empty()) {
        ui_.drawTextCentered(resultMsg,
                              { PX + 20, PY + 80, PW - 40, PH - 120 }, COL_TEXT);
        ui_.drawText("[Enter / Space] Continue",
                      PX + 30, PY + PH - 30, COL_DIM);
        return;
    }

    // Intelligence section
    int iy = PY + 70;
    bool hasLookout = captain_.hasLookout || captain_.hasSpotter;
    bool hasSpotter = captain_.hasSpotter;

    if (!hasLookout) {
        ui_.drawText("An unknown vessel is bearing down on you.", PX + 30, iy, COL_TEXT);
        iy += 30;
        ui_.drawText("(Hire a Lookout in port to identify ships.)", PX + 30, iy, COL_DIM);
        iy += 30;
    } else {
        std::string flagLine = "Flag:    ";
        flagLine += nationName(enemy_.displayNation);
        if (enemy_.displayNation != enemy_.nation) flagLine += "  [false colours?]";
        ui_.drawText(flagLine,                                            PX + 30, iy, COL_TEXT); iy += 28;
        ui_.drawText("Faction: " + std::string(factionLabel(enemy_.faction)), PX + 30, iy, COL_TEXT); iy += 28;
        ui_.drawText("Class:   " + std::string(classLabel(enemy_.shipClass)), PX + 30, iy, COL_TEXT); iy += 28;

        if (hasSpotter) {
            iy += 4;
            ui_.drawText("Hull:    " + std::to_string(enemy_.hullCur) + " / " + std::to_string(enemy_.hullMax),
                          PX + 30, iy, COL_TEXT); iy += 26;
            ui_.drawText("Crew:    " + std::to_string(enemy_.crewCur),   PX + 30, iy, COL_TEXT); iy += 26;
            ui_.drawText("Cannons: " + std::to_string(enemy_.cannonCount), PX + 30, iy, COL_TEXT); iy += 26;
            if (enemy_.faction == Faction::Merchant)
                ui_.drawText("Cargo:   ~" + std::to_string(enemy_.cargoCur) + " units",
                              PX + 30, iy, COL_TEXT);
            iy += 26;
        }
    }

    // Action menu
    int menuY = PY + PH - 210;
    SDL_Rect divider{ PX + 20, menuY - 8, PW - 40, 1 };
    SDL_SetRenderDrawColor(ui_.renderer(), 70, 80, 100, 255);
    SDL_RenderFillRect(ui_.renderer(), &divider);

    ui_.drawText("Choose your action:", PX + 30, menuY + 4, COL_TITLE);
    menuY += 34;

    const auto& items = menu_.items();
    for (int i = 0; i < (int)items.size(); ++i) {
        SDL_Rect itemR{ PX + 30, menuY + i * 48, PW - 60, 44 };
        ui_.drawMenuItem(items[i].label, itemR, i == menu_.selectedIndex());
    }

    ui_.drawText("[Up/Down] Navigate    [Enter] Confirm",
                  PX + 30, PY + PH - 26, COL_DIM);
}
