#pragma once
#include "ui/UIRenderer.h"
#include "ui/Menu.h"
#include "ship/ShipStats.h"
#include "world/World.h"
#include "world/Port.h"
#include "captain/Captain.h"

class InputState;

enum class PortView {
    MainMenu, Tavern, Shipyard, TradingPost, GovernorsMansion
};

class PortScreen {
public:
    PortScreen(UIRenderer& ui, ShipStats& ship, Captain& captain,
               const Town& town, Port& port);

    void handleInput(const InputState& input);
    void render();

    bool wantsToLeave() const { return wantsToLeave_; }

private:
    UIRenderer& ui_;
    ShipStats&  ship_;
    Captain&    captain_;
    const Town& town_;
    Port&       port_;

    PortView currentView_ = PortView::MainMenu;
    Menu     mainMenu_;
    Menu     subMenu_;
    bool     wantsToLeave_ = false;

    std::string statusMsg_;   // confirmation / error message shown in sub-views

    static constexpr int PX = 200, PY = 80, PW = 880, PH = 560;
    static constexpr int TITLE_H = 64;
    static constexpr int ITEM_H  = 50;
    static constexpr int MENU_W  = 360;

    SDL_Rect panelRect()   const { return { PX, PY, PW, PH }; }
    SDL_Rect titleRect()   const { return { PX, PY, PW, TITLE_H }; }
    SDL_Rect contentRect() const { return { PX, PY + TITLE_H, PW, PH - TITLE_H }; }

    void handleMainMenu(const InputState& input);
    void handleShipyard(const InputState& input);
    void handleTavern  (const InputState& input);
    void handleSubView (const InputState& input);   // generic fallback

    void renderMainMenu();
    void renderShipyard();
    void renderTavern();
    void renderSubView(const std::string& subTitle, const std::string& body);

    void enterSubView(PortView view);
    void buildShipyardMenu();
    void buildTavernMenu();
};
