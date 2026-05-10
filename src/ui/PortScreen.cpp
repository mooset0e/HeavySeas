#include "ui/PortScreen.h"
#include "core/InputState.h"
#include "core/Constants.h"

static const SDL_Color COL_BG     = {  8, 16, 38, 240 };
static const SDL_Color COL_BORDER = { 70, 120, 155, 255 };
static const SDL_Color COL_TITLE  = { 255, 220, 80, 255 };
static const SDL_Color COL_TEXT   = { 200, 180, 140, 255 };
static const SDL_Color COL_DIM    = {  90, 100, 100, 255 };

PortScreen::PortScreen(UIRenderer& ui, ShipStats& ship, const Town& town, Port& port)
    : ui_(ui), ship_(ship), town_(town), port_(port)
{
    mainMenu_ = Menu({
        { "The Tavern",             "Recruit crew and hear rumors of treasure." },
        { "The Shipyard",           "Repair your vessel and check your cannons." },
        { "The Trading Post",       "Buy and sell cargo for profit." },
        { "The Governor's Mansion", "Receive missions and letters of marque." },
        { "Set Sail",               "Return to the open sea." },
    });
    subMenu_ = Menu({ { "Leave", "" } });
}

void PortScreen::handleInput(const InputState& input) {
    if (currentView_ == PortView::MainMenu)
        handleMainMenu(input);
    else
        handleSubView(input);
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
}

void PortScreen::handleSubView(const InputState& input) {
    if (input.justPressed(SDL_SCANCODE_ESCAPE) ||
        (input.justPressed(SDL_SCANCODE_RETURN) && subMenu_.selectedIndex() == 0))
    {
        currentView_ = PortView::MainMenu;
        subMenu_ = Menu({ { "Leave", "" } });
    }
}

void PortScreen::enterSubView(PortView view) {
    currentView_ = view;
    subMenu_ = Menu({ { "Leave", "" } });
}

void PortScreen::render() {
    // Dark overlay behind panel
    SDL_SetRenderDrawBlendMode(ui_.renderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ui_.renderer(), 0, 0, 0, 160);
    SDL_Rect fullscreen{ 0, 0, SCREEN_W, SCREEN_H };
    SDL_RenderFillRect(ui_.renderer(), &fullscreen);

    // Main panel
    ui_.drawPanel(panelRect(), COL_BG, COL_BORDER, 2);

    // Title bar
    ui_.drawPanel(titleRect(), { 15, 25, 55, 255 }, COL_BORDER, 2);
    ui_.drawTextCentered(town_.name, titleRect(), COL_TITLE);

    if (currentView_ == PortView::MainMenu)
        renderMainMenu();
    else {
        std::string subTitle, body;
        switch (currentView_) {
            case PortView::Tavern:
                subTitle = "The Tavern";
                body = "The tavern is thick with pipe smoke and sea shanties.\n"
                       "A one-eyed barkeep slides you a rum without being asked.\n\n"
                       "Crew:  " + std::to_string(ship_.crewCur) + " / " + std::to_string(ship_.crewMax);
                break;
            case PortView::Shipyard:
                subTitle = "The Shipyard";
                body = "A weathered shipwright looks your hull over with a squint.\n\n"
                       "Hull:    " + std::to_string(ship_.hullCur) + " / " + std::to_string(ship_.hullMax) + "\n"
                       "Cannons: " + std::to_string(ship_.cannons) + "\n"
                       "Crew:    " + std::to_string(ship_.crewCur) + " / " + std::to_string(ship_.crewMax);
                break;
            case PortView::TradingPost:
                subTitle = "The Trading Post";
                body = "Barrels and crates line the walls floor to ceiling.\n\n"
                       "Cargo: " + std::to_string(ship_.cargoCur) + " / " + std::to_string(ship_.cargoMax) + "\n"
                       "Gold:  " + std::to_string(ship_.gold);
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

    // Description of selected item on the right
    SDL_Rect descRect{ PX + MENU_W + 40, content.y + 30, PW - MENU_W - 60, content.h - 60 };
    ui_.drawPanel(descRect, { 12, 22, 50, 200 }, COL_BORDER, 1);
    ui_.drawText(mainMenu_.selected().description, descRect.x + 14, descRect.y + 16, COL_TEXT);

    // Footer hint
    ui_.drawText("[Up/Down] Navigate    [Enter] Select    [Esc] Set Sail",
                 PX + 20, PY + PH - 28, COL_DIM);
}

void PortScreen::renderSubView(const std::string& subTitle, const std::string& body) {
    SDL_Rect content = contentRect();

    // Sub-title
    SDL_Rect subTitleRect{ content.x + 20, content.y + 16, content.w - 40, 30 };
    ui_.drawText(subTitle, subTitleRect.x, subTitleRect.y, COL_TITLE);

    // Body text — split on \n and draw line by line
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

    // Leave button
    SDL_Rect leaveRect{ PX + 20, PY + PH - 70, 200, ITEM_H };
    ui_.drawMenuItem("Leave", leaveRect, true);

    ui_.drawText("[Enter/Esc] Leave", PX + 20, PY + PH - 28, COL_DIM);
}
