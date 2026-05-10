#include "ui/PortScreen.h"
#include "core/InputState.h"
#include "core/Constants.h"
#include "ship/ShipType.h"
#include <algorithm>

static const SDL_Color COL_BG     = {  8, 16, 38, 240 };
static const SDL_Color COL_BORDER = { 70, 120, 155, 255 };
static const SDL_Color COL_TITLE  = { 255, 220, 80, 255 };
static const SDL_Color COL_TEXT   = { 200, 180, 140, 255 };
static const SDL_Color COL_DIM    = {  90, 100, 100, 255 };
static const SDL_Color COL_OK     = {  80, 200, 100, 255 };
static const SDL_Color COL_ERR    = { 220,  80,  60, 255 };

// ---- construction ----

PortScreen::PortScreen(UIRenderer& ui, ShipStats& ship, Captain& captain,
                        const Town& town, Port& port)
    : ui_(ui), ship_(ship), captain_(captain), town_(town), port_(port)
{
    mainMenu_ = Menu({
        { "The Tavern",             "Recruit crew and hear rumors of treasure." },
        { "The Shipyard",           "Repair your vessel and hire crew." },
        { "The Trading Post",       "Buy and sell cargo for profit." },
        { "The Governor's Mansion", "Receive missions and letters of marque." },
        { "Set Sail",               "Return to the open sea." },
    });
    subMenu_ = Menu({ { "Leave", "" } });
}

// ---- input routing ----

void PortScreen::handleInput(const InputState& input) {
    if      (currentView_ == PortView::MainMenu)  handleMainMenu(input);
    else if (currentView_ == PortView::Shipyard)  handleShipyard(input);
    else if (currentView_ == PortView::Tavern)    handleTavern(input);
    else                                           handleSubView(input);
}

void PortScreen::handleMainMenu(const InputState& input) {
    if (input.justPressed(SDL_SCANCODE_UP))   mainMenu_.moveUp();
    if (input.justPressed(SDL_SCANCODE_DOWN)) mainMenu_.moveDown();
    if (input.justPressed(SDL_SCANCODE_RETURN)) {
        switch (mainMenu_.selectedIndex()) {
            case 0: enterSubView(PortView::Tavern);           break;
            case 1: enterSubView(PortView::Shipyard);         break;
            case 2: enterSubView(PortView::TradingPost);      break;
            case 3: enterSubView(PortView::GovernorsMansion); break;
            case 4: wantsToLeave_ = true;                     break;
        }
    }
    if (input.justPressed(SDL_SCANCODE_ESCAPE)) wantsToLeave_ = true;
}

void PortScreen::handleSubView(const InputState& input) {
    if (input.justPressed(SDL_SCANCODE_ESCAPE) ||
        (input.justPressed(SDL_SCANCODE_RETURN) && subMenu_.selectedIndex() == 0))
    {
        currentView_ = PortView::MainMenu;
        statusMsg_.clear();
        subMenu_ = Menu({ { "Leave", "" } });
    }
}

void PortScreen::handleShipyard(const InputState& input) {
    if (input.justPressed(SDL_SCANCODE_ESCAPE)) {
        currentView_ = PortView::MainMenu;
        statusMsg_.clear();
        return;
    }
    if (input.justPressed(SDL_SCANCODE_UP))   subMenu_.moveUp();
    if (input.justPressed(SDL_SCANCODE_DOWN)) subMenu_.moveDown();

    if (input.justPressed(SDL_SCANCODE_RETURN)) {
        int sel = subMenu_.selectedIndex();
        const auto& items = subMenu_.items();

        // Last item is always "Leave"
        if (sel == (int)items.size() - 1) {
            currentView_ = PortView::MainMenu;
            statusMsg_.clear();
            return;
        }

        if (items[sel].label.rfind("Repair", 0) == 0) {
            int missing = ship_.hullMax - ship_.hullCur;
            int cost    = missing * port_.repairCostPerHP;
            if (missing == 0) {
                statusMsg_ = "Hull is already at full strength.";
            } else if (ship_.gold < cost) {
                statusMsg_ = "Not enough gold! Need " + std::to_string(cost) + "g.";
            } else {
                ship_.gold   -= cost;
                ship_.hullCur = ship_.hullMax;
                statusMsg_    = "Hull repaired to full. Cost: " + std::to_string(cost) + "g.";
                buildShipyardMenu();
            }
        } else if (items[sel].label.rfind("Hire", 0) == 0) {
            int need = ship_.crewMax - ship_.crewCur;
            int cost = need * port_.hireCostPerCrew;
            if (need == 0) {
                statusMsg_ = "Crew is already at full complement.";
            } else if (ship_.gold < cost) {
                statusMsg_ = "Not enough gold! Need " + std::to_string(cost) + "g.";
            } else {
                ship_.gold   -= cost;
                ship_.crewCur = ship_.crewMax;
                statusMsg_    = "Crew hired to full. Cost: " + std::to_string(cost) + "g.";
                buildShipyardMenu();
            }
        }
    }
}

void PortScreen::handleTavern(const InputState& input) {
    if (input.justPressed(SDL_SCANCODE_ESCAPE)) {
        currentView_ = PortView::MainMenu;
        statusMsg_.clear();
        return;
    }
    if (input.justPressed(SDL_SCANCODE_UP))   subMenu_.moveUp();
    if (input.justPressed(SDL_SCANCODE_DOWN)) subMenu_.moveDown();

    if (input.justPressed(SDL_SCANCODE_RETURN)) {
        int sel = subMenu_.selectedIndex();
        const auto& items = subMenu_.items();

        if (sel == (int)items.size() - 1) {
            currentView_ = PortView::MainMenu;
            statusMsg_.clear();
            return;
        }

        if (items[sel].label.rfind("Hire Lookout", 0) == 0) {
            int cost = 300;
            if (captain_.hasLookout) {
                statusMsg_ = "You already have a Lookout.";
            } else if (ship_.gold < cost) {
                statusMsg_ = "Not enough gold! Need " + std::to_string(cost) + "g.";
            } else {
                ship_.gold        -= cost;
                captain_.hasLookout = true;
                statusMsg_          = "Lookout hired for " + std::to_string(cost) + "g.";
                buildTavernMenu();
            }
        } else if (items[sel].label.rfind("Hire Spotter", 0) == 0) {
            int cost = 800;
            if (!captain_.hasLookout) {
                statusMsg_ = "Requires a Lookout first.";
            } else if (captain_.hasSpotter) {
                statusMsg_ = "You already have a Spotter.";
            } else if (ship_.gold < cost) {
                statusMsg_ = "Not enough gold! Need " + std::to_string(cost) + "g.";
            } else {
                ship_.gold        -= cost;
                captain_.hasSpotter = true;
                statusMsg_          = "Spotter hired for " + std::to_string(cost) + "g.";
                buildTavernMenu();
            }
        }
    }
}

// ---- sub-view builders ----

void PortScreen::buildShipyardMenu() {
    int missingHull = ship_.hullMax - ship_.hullCur;
    int missingCrew = ship_.crewMax - ship_.crewCur;
    int repairCost  = missingHull * port_.repairCostPerHP;
    int hireCost    = missingCrew * port_.hireCostPerCrew;

    std::vector<MenuItem> items;
    items.push_back({ "Repair Hull  (" + std::to_string(repairCost) + "g)", "" });
    items.push_back({ "Hire Crew    (" + std::to_string(hireCost)   + "g)", "" });
    items.push_back({ "Leave", "" });
    subMenu_ = Menu(items);
}

void PortScreen::buildTavernMenu() {
    std::vector<MenuItem> items;
    if (!captain_.hasLookout)
        items.push_back({ "Hire Lookout (300g)", "Reveals enemy flag, faction and size at range." });
    if (captain_.hasLookout && !captain_.hasSpotter)
        items.push_back({ "Hire Spotter (800g)", "Reveals full enemy details, sees through false colors." });
    items.push_back({ "Leave", "" });
    subMenu_ = Menu(items);
}

void PortScreen::enterSubView(PortView view) {
    currentView_ = view;
    statusMsg_.clear();
    if      (view == PortView::Shipyard) buildShipyardMenu();
    else if (view == PortView::Tavern)   buildTavernMenu();
    else subMenu_ = Menu({ { "Leave", "" } });
}

// ---- rendering ----

void PortScreen::render() {
    SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ui_.renderer(), 0, 0, 0, 160);
    SDL_Rect full{ 0, 0, SCREEN_W, SCREEN_H };
    SDL_RenderFillRect(ui_.renderer(), &full);

    ui_.drawPanel(panelRect(), COL_BG, COL_BORDER, 2);
    ui_.drawPanel(titleRect(), { 15, 25, 55, 255 }, COL_BORDER, 2);
    ui_.drawTextCentered(town_.name + "  [" + nationName(town_.nation) + "]",
                          titleRect(), COL_TITLE);

    switch (currentView_) {
        case PortView::MainMenu:  renderMainMenu(); break;
        case PortView::Shipyard:  renderShipyard(); break;
        case PortView::Tavern:    renderTavern();   break;
        default: {
            std::string subTitle, body;
            switch (currentView_) {
                case PortView::TradingPost:
                    subTitle = "The Trading Post";
                    body = "Barrels and crates line the walls floor to ceiling.\n\n"
                           "Cargo: " + std::to_string(ship_.cargoCur) + " / " + std::to_string(ship_.cargoMax) + "\n"
                           "Gold:  " + std::to_string(ship_.gold) + "\n\n"
                           "(Trading not yet implemented.)";
                    break;
                case PortView::GovernorsMansion:
                    subTitle = "The Governor's Mansion";
                    body = "The governor greets you with a cautious handshake.\n\n"
                           "\"We have need of a captain of your... particular skills.\"\n\n"
                           "No missions available at this time.";
                    break;
                default: break;
            }
            renderSubView(subTitle, body);
            break;
        }
    }
}

void PortScreen::renderMainMenu() {
    SDL_Rect content = contentRect();
    int startY = content.y + 30;

    const auto& items = mainMenu_.items();
    for (int i = 0; i < (int)items.size(); ++i) {
        SDL_Rect itemRect{ PX + 20, startY + i * ITEM_H, MENU_W, ITEM_H };
        ui_.drawMenuItem(items[i].label, itemRect, i == mainMenu_.selectedIndex());
    }

    SDL_Rect descRect{ PX + MENU_W + 40, content.y + 30, PW - MENU_W - 60, content.h - 60 };
    ui_.drawPanel(descRect, { 12, 22, 50, 200 }, COL_BORDER, 1);
    ui_.drawText(mainMenu_.selected().description, descRect.x + 14, descRect.y + 16, COL_TEXT);

    ui_.drawText("[Up/Down] Navigate    [Enter] Select    [Esc] Set Sail",
                 PX + 20, PY + PH - 28, COL_DIM);
}

void PortScreen::renderShipyard() {
    SDL_Rect content = contentRect();

    ui_.drawText("The Shipyard", content.x + 20, content.y + 16, COL_TITLE);

    // Stats summary
    int iy = content.y + 52;
    ui_.drawText("Hull:  " + std::to_string(ship_.hullCur) + " / " + std::to_string(ship_.hullMax),
                 content.x + 20, iy, COL_TEXT); iy += 26;
    ui_.drawText("Crew:  " + std::to_string(ship_.crewCur) + " / " + std::to_string(ship_.crewMax),
                 content.x + 20, iy, COL_TEXT); iy += 26;
    ui_.drawText("Gold:  " + std::to_string(ship_.gold),
                 content.x + 20, iy, COL_TEXT); iy += 40;

    const auto& items = subMenu_.items();
    for (int i = 0; i < (int)items.size(); ++i) {
        SDL_Rect ir{ content.x + 20, iy + i * ITEM_H, MENU_W, ITEM_H };
        ui_.drawMenuItem(items[i].label, ir, i == subMenu_.selectedIndex());
    }

    if (!statusMsg_.empty()) {
        bool isErr = statusMsg_.find("Not enough") != std::string::npos ||
                     statusMsg_.find("already")    != std::string::npos;
        ui_.drawText(statusMsg_, content.x + 20, PY + PH - 56,
                     isErr ? COL_ERR : COL_OK);
    }

    ui_.drawText("[Up/Down] Navigate    [Enter] Select    [Esc] Leave",
                 PX + 20, PY + PH - 28, COL_DIM);
}

void PortScreen::renderTavern() {
    SDL_Rect content = contentRect();

    ui_.drawText("The Tavern", content.x + 20, content.y + 16, COL_TITLE);

    int iy = content.y + 52;
    ui_.drawText("The tavern is thick with pipe smoke and sea shanties.", content.x + 20, iy, COL_TEXT); iy += 28;
    ui_.drawText("A one-eyed barkeep slides you a rum without being asked.", content.x + 20, iy, COL_TEXT); iy += 44;

    std::string crew = "Lookout: " + std::string(captain_.hasLookout ? "Hired" : "None") +
                       "   Spotter: " + std::string(captain_.hasSpotter ? "Hired" : "None");
    ui_.drawText(crew, content.x + 20, iy, COL_TEXT); iy += 40;

    const auto& items = subMenu_.items();
    for (int i = 0; i < (int)items.size(); ++i) {
        SDL_Rect ir{ content.x + 20, iy + i * ITEM_H, MENU_W + 60, ITEM_H };
        ui_.drawMenuItem(items[i].label, ir, i == subMenu_.selectedIndex());
    }

    if (!statusMsg_.empty()) {
        bool isErr = statusMsg_.find("Not enough") != std::string::npos ||
                     statusMsg_.find("already")    != std::string::npos ||
                     statusMsg_.find("Requires")   != std::string::npos;
        ui_.drawText(statusMsg_, content.x + 20, PY + PH - 56,
                     isErr ? COL_ERR : COL_OK);
    }

    ui_.drawText("[Up/Down] Navigate    [Enter] Select    [Esc] Leave",
                 PX + 20, PY + PH - 28, COL_DIM);
}

void PortScreen::renderSubView(const std::string& subTitle, const std::string& body) {
    SDL_Rect content = contentRect();

    ui_.drawText(subTitle, content.x + 20, content.y + 16, COL_TITLE);

    int lineY = content.y + 60;
    std::string line;
    for (char c : body) {
        if (c == '\n') {
            if (!line.empty()) ui_.drawText(line, content.x + 20, lineY, COL_TEXT);
            lineY += 28;
            line.clear();
        } else {
            line += c;
        }
    }
    if (!line.empty()) ui_.drawText(line, content.x + 20, lineY, COL_TEXT);

    SDL_Rect leaveRect{ PX + 20, PY + PH - 70, 200, ITEM_H };
    ui_.drawMenuItem("Leave", leaveRect, true);
    ui_.drawText("[Enter/Esc] Leave", PX + 20, PY + PH - 28, COL_DIM);
}
