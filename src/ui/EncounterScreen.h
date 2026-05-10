#pragma once
#include <vector>
#include "ui/UIRenderer.h"
#include "ui/Menu.h"
#include "ship/EnemyShip.h"
#include "ship/ShipStats.h"
#include "captain/Captain.h"

class InputState;

enum class EncounterChoice { None, Engage, Hail, Rob, Flee };

class EncounterScreen {
public:
    EncounterScreen(UIRenderer& ui, const EnemyShip& enemy,
                    const ShipStats& player, const Captain& captain);

    void handleInput(const InputState& input);
    void render();

    EncounterChoice choice() const { return choice_; }

    // For reporting what happened after Hail/Rob (caller sets this)
    std::string resultMsg;   // if non-empty, shown instead of menu

private:
    UIRenderer&      ui_;
    const EnemyShip& enemy_;
    const ShipStats& player_;
    const Captain&   captain_;

    Menu                        menu_;
    std::vector<EncounterChoice> itemChoices_;
    EncounterChoice              choice_ = EncounterChoice::None;

    static constexpr int PX = 300, PY = 120, PW = 680, PH = 480;
};
